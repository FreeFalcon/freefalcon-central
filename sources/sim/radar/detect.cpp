#include "stdhdr.h"
#include "entity.h"
#include "simmath.h"
#include "f4vu.h"
#include "debuggr.h"
#include "object.h"
#include "simbase.h"
#include "otwdrive.h"
#include "Entity.h"
#include "campbase.h"
#include "radarDoppler.h"
#include "simmover.h"//me123

/* 2001-09-07 S.G. RP5 */ extern bool g_bRP5Comp;

static const float APG68_PULSE_WIDTH	= 2.0e-5f;

int RadarDopplerClass::InResCell (SimObjectType* rdrObj, int i, int *rngCell,
	int *angCell, int *velCell)
{
float cellAngResolution, cellRngResolution,velResolution;
int detflag, j, k, inRangeCell, inAzcell;

	detflag = TRUE;
   /*-------------------------------*/
   /* resolution cell dimensions    */
   /* ang = 2*R*tan(beamWidth/2)   */
   /* rng = ct/2                    */
   /*-------------------------------*/
   cellAngResolution = 2.0F * (float)tan(beamWidth) *
		 max (10.0F, rdrObj->localData->range);
   cellRngResolution = LIGHTSPEED * 0.5F * APG68_PULSE_WIDTH;
   velResolution = 40.0f * FTPSEC_TO_KNOTS;

   /*-------------------*/
   /* assign range cell */
   /*-------------------*/
   j = FloatToInt32(rdrObj->localData->range / cellRngResolution);

   /*----------------------------------*/
   /* check others for same range cell */
   /*----------------------------------*/
   inRangeCell = FALSE;
   for (k=0; k<i; k++)
   {
	   if (rngCell[k] == j)
      {
     		inRangeCell = TRUE;
         break;
      }
   }

	rngCell[i] = j;

    inAzcell = FALSE;
	if( inRangeCell)
	{
     	/*-------------------*/
     	/* assign angle cell */
     	/*-------------------*/
	  	j = (int)(rdrObj->localData->az / cellAngResolution);

     	/*----------------------------------*/
     	/* check others for same angle cell */
     	/*----------------------------------*/
     	for (k=0; k<i; k++)
     	{
        	if (angCell[k] == j)
        	{
				inAzcell = TRUE;
           	break;
        	}
     	}
		angCell[i] = j;
	}

	if (inAzcell)
	{
     	/*-------------------*/
     	/* assign velosity cell */
     	/*-------------------*/
	  	j = (int) ((float)(cos(rdrObj->localData->ataFrom) * rdrObj->BaseData()->GetVt()) / velResolution);

     	/*----------------------------------*/
     	/* check others for same angle cell */
     	/*----------------------------------*/
     	for (k=0; k<i; k++)
     	{
        	if (velCell[k] == j)
        	{
				detflag = FALSE;
           	break;
        	}
     	}
		angCell[i] = j;
	}

	return (!detflag);
//   return FALSE;
}


int RadarDopplerClass::ObjectDetected (SimObjectType* obj)
{
	float	S = ReturnStrength( obj );


// 2000-09-22 MODIFIED BY S.G. SO THE MODE AFFECTS THE SIGNAL STRENGTH IN A DIFFERENT WAY
// THE MODS ARE

// With radar lock/bugged target:
// RWS/SAM:	1.1
// TWS:		1.0
// VS:		1.2
// STT:		1.4

// No radar lock/bugged target:
// RWS/SAM:	1.0
// TWS:		0.9
// VS:		1.2
// STT:		1.4


// This function only runs in Air to Air radar mode for the player

// If 'STTingTarget' is set, we have a locked target in STT mode
// otherwise were in a different mode as stated by the 'mode' variable


	// Test for being in STT mode
	if (IsSet(STTingTarget))
		S *= 1.4f;

	// Either RWS or SAM mode. If we have a focus on a target, increase signal strength a bit
	else if (mode == RWS || mode == SAM) {
		if (obj == lockedTarget)
			S *= 1.1f;
		else
			S *= 1.0f;
		if (mode == SAM)
			{
			 SimObjectLocalData* rdrData;
			 rdrData = obj->localData;	
		 // if the expected possition is too far from the actual hammer signal to zero
			if (rdrData->rdrSy[0]&&rdrData->rdrSy[1])// we have a track history
				{
				 float expectedY =	 rdrData->rdrY[0]  + (rdrData->rdrY[0] - rdrData->rdrY[1]);
				 float expectedX=	 (rdrData->rdrX[0]+ (rdrData->rdrHd[0] - platform->Yaw()))+
									(
										(rdrData->rdrX[0]+ (rdrData->rdrHd[0] - platform->Yaw()))- 
										(rdrData->rdrX[1]+ (rdrData->rdrHd[1] - platform->Yaw()))
									);

				 float realaz = rdrData->rdrX[0] + (rdrData->rdrHd[0] - platform->Yaw());
				 realaz = RES180(realaz);

				 if (radarDatFile && (fabs(realaz - expectedX) > radarDatFile->MaxAngleDiffSam *DTR||
					 (fabs(obj->localData->range - expectedY) )>   (expectedY * radarDatFile->MaxRangeDiffSam)/ 100.0f))
					 {
						 S*=0.0f;//hammer to zero
						 ExtrapolateHistory (obj);
					 }
				}
			}
	}
	else if (mode == LRS) { // boost a bit
		if (obj == lockedTarget)
			S *= 1.2f;
		else
			S *= 1.1f;
	}
	// TWS and VS mode signal strength multiplier
	else if (mode == TWS) {
		if (obj != lockedTarget)
			S *= 0.9f;
		 SimObjectLocalData* rdrData;
		 rdrData = obj->localData;	
	 // if the expected possition is too far from the actual hammer signal to zero
		if (rdrData->rdrSy[0] &&rdrData->rdrSy[1])// we have a track history
		{
				 float expectedY =	 rdrData->rdrY[0]  + (rdrData->rdrY[0] - rdrData->rdrY[1]);
				 float expectedX=	 (rdrData->rdrX[1]+ (rdrData->rdrHd[1] - platform->Yaw()))+
									(
										(rdrData->rdrX[1]+ (rdrData->rdrHd[1] - platform->Yaw()))- 
										(rdrData->rdrX[2]+ (rdrData->rdrHd[2] - platform->Yaw()))
									);

				 float realaz = rdrData->rdrX[1] + (rdrData->rdrHd[1] - platform->Yaw());
				 realaz = RES180(realaz);

		 if (radarDatFile && (fabs(realaz - expectedX) > radarDatFile->MaxAngleDiffTws *DTR||
			 (fabs(obj->localData->range - expectedY) )>   (expectedY * radarDatFile->MaxRangeDiffTws)/ 100.0f))
		 {
			 S*=0.0f;//hammer to zero
			 if (rdrData->rdrSy[1] == Track ) rdrData->rdrSy[0] = FlashTrack;
			 if (rdrData->rdrSy[1] == Bug)rdrData->rdrSy[0] = FlashBug;
			 if (rdrData->rdrSy[1] == AimRel)rdrData->rdrSy[0] = AimFlash;
			 if (rdrData->rdrSy[1] == FlashTrack ||  
				 rdrData->rdrSy[1] == FlashBug  || 
				 rdrData->rdrSy[1] == AimFlash)  
				 rdrData->rdrSy[0] = Det; 
			 ExtrapolateHistory (obj);
		 }
		}

	 
	}
	else if (mode == VS)
		S *= 1.2f;


	if (1)// me123 agreed with jjb to test this !g_bRP5Comp) 
	{
		VU_ID lastChaffID			= FalconNullId;
		VU_ID			id;
		FalconEntity	*cm;
		float			chance;
		int				dummy = 0;
		SimObjectType *target = obj;
		static const float	cmRangeArray[]		= {0.0F,  1500.0f,  3000.0f,  11250.0f,  18750.0f,  30000.0f};
		static const float	cmBiteChanceArray[]	= {0.0F,     0.1F,     0.5F,      0.5F,      0.2F,      0.1F};
		static const int	cmArrayLength		= sizeof(cmRangeArray) / sizeof(cmRangeArray[0]);
	
		// No counter measures deployed by campaign things
	  // countermeasures only work when tracking (for now)
		//MI possible CTD? added !target->BaseData() check
	
		if (!lockedTarget || !target || !target->BaseData() ||!target->BaseData()->IsSim()) {
			return ( S >= 0.8f + 0.4f*(float)rand()/BIGGEST_RANDOM_NUMBER);
		}
	
		// Get the ID of the most recently launched counter measure from our target
		id = ((SimBaseClass*)target->BaseData())->NewestChaffID();
		
		// If we have a new chaff bundle to deal with
		if (id != lastChaffID)
		{
			// Stop here if there isn't a counter measure in play
			if (id == FalconNullId) {
				lastChaffID = id;
	
				return ( S >= 0.8f + 0.4f*(float)rand()/BIGGEST_RANDOM_NUMBER);
			}
		
			// Try to find the counter measure entity in the database
			cm = (FalconEntity*)vuDatabase->Find( id );

	//		MonoPrint ("ConsiderDecoy %08x %f: ", cm, target->localData->range);

			if (!cm) {
				// We'll have to wait until next time
				// (probably because the create event hasn't been processed locally yet)
				return ( S >= 0.8f + 0.4f*(float)rand()/BIGGEST_RANDOM_NUMBER);
			}
	
			// Start with the suceptability of this seeker to counter measures
			chance = radarData->ChaffChance;

			// Adjust with a range to target based chance of an individual countermeasure working
			chance *= Math.OnedInterp(target->localData->range, cmRangeArray, cmBiteChanceArray, cmArrayLength, &dummy);
			float Vr =(float)cos(target->localData->ataFrom) * target->BaseData()->GetVt();
			if (fabs(Vr) > radarData->NotchSpeed)  
				chance = min (0.95f, chance * radarData->NotchPenalty);
			
			// Roll the dice
			if (chance > (float)rand()/RAND_MAX)
			{
				// Compute some relative geometry stuff
				const float atx	= platform->dmx[0][0];
				const float aty	= platform->dmx[0][1];
				const float atz	= platform->dmx[0][2];
				const float dx	= cm->XPos() - platform->XPos();
				const float dy	= cm->YPos() - platform->YPos();
				const float dz	= cm->ZPos() - platform->ZPos();
				const float range	= (float)sqrt( dx*dx + dy*dy );
				const float cosATA	= (atx*dx + aty*dy + atz*dz) / (float)sqrt(range*range+dz*dz);
				
				// Only take the bait if we can see the thing
				// TODO:  Should probably use beam width instead of scan angle...
				if (cosATA >= cos(radarData->ScanHalfAngle))
				{
			  ClearSensorTarget();
				}
				MonoPrint ("Yes %f %08x\n", chance, target);
			}
			else
			{
	//			MonoPrint ("No %f %08x\n", chance, target);
			}

			// Note that we've considered this countermeasure
			lastChaffID = id;
		}
	}

	return ( S >= 0.8f + 0.4f*(float)rand()/BIGGEST_RANDOM_NUMBER);



	
}
