#include "stdhdr.h"
#include "f4vu.h"
#include "object.h"
#include "sensors.h"
#include "radarDoppler.h"
#include "geometry.h"
#include "simmover.h"
#include "simdrive.h"
//MI
#include "aircrft.h"

//JAM 13Oct03
extern float g_fDBS1ScanRateFactor;
extern float g_fDBS2ScanRateFactor;
//JAM

void RadarDopplerClass::MoveBeam (void)
{
float curScanRate;
float az, el, theta;

	//MI no radar if RF Switch in SILENT or QUIET
	if(SimDriver.GetPlayerAircraft() && (SimDriver.GetPlayerAircraft()->RFState == 1 ||
		SimDriver.GetPlayerAircraft()->RFState == 2))
	{
		SetEmitting(FALSE);
	}

	// Seems like most of this could be done once at mode change or in per mode processing???
	switch(mode)
	{
	  case GM:
		if (flags & DBS1)
		{
			curScanRate = scanRate * g_fDBS1ScanRateFactor;//0.25F;	// We're fudging -- Up the scan rate to reduce the latency
		}
		else if (flags & DBS2)
		{
			curScanRate = scanRate * g_fDBS2ScanRateFactor;//0.05F;	// We're fudging -- Up the scan rate to reduce the latency
		}
		else
		{
			curScanRate = scanRate * 2.0f;	// We're fudging -- Up the scan rate to reduce the latency
		}
		break;

	  case LRS:
	      curScanRate = scanRate * 0.5f; // JPO a bit slower
	  break;
	  case SAM:
		  if(prevMode == LRS)
			  curScanRate = scanRate * 0.5f; // JPO a bit slower
		  else
			  curScanRate = scanRate;
	  break;
	  default:
		curScanRate = scanRate;
		break;
	}


   if (scanDir == ScanNone)
   {
	   curScanLeft   = beamAz - beamWidth;
	   curScanRight  = beamAz + beamWidth;
	   curScanTop    = beamEl + beamWidth;
	   curScanBottom = beamEl - beamWidth;
   }
   else if (IsSet(HomingBeam))
   {
      if (beamAz > -azScan)
      {
			beamAz -= curScanRate * SimLibMajorFrameTime;
         if (beamAz < -azScan)
            beamAz = -azScan;
      }
      else if (beamAz < -azScan)
      {
			beamAz += curScanRate * SimLibMajorFrameTime;
         if (beamAz > -azScan)
            beamAz = -azScan;
      }

      if (beamEl > elScan)
      {
			beamEl -= curScanRate * SimLibMajorFrameTime;
         if (beamEl < elScan)
            beamEl = elScan;
      }
      else if (beamEl < elScan)
      {
			beamEl += curScanRate * SimLibMajorFrameTime;
         if (beamEl > elScan)
            beamEl = elScan;
      }

      if (beamAz == -azScan && beamEl == elScan)
         ClearFlagBit(HomingBeam);
   }
   /*-----------------*/
   /* Vertical search */
   /*-----------------*/
   else if (IsSet(VerticalScan))
   {
	   /*----------------*/
	   /* Inbetween bars */
	   /*----------------*/
	   if (IsSet(ChangingBars))
	   {
		   curScanTop    = beamEl + beamWidth;
		   curScanBottom = beamEl - beamWidth;
		   /*-------------*/
		   /* Going Right */
		   /*-------------*/
		   if (targetAz > beamAz)
		   {
			   curScanLeft = beamAz - beamWidth;
			   beamAz += curScanRate * SimLibMajorFrameTime;
			   if (beamAz >= targetAz)
			   {
				   beamAz = targetAz;
              ClearFlagBit(ChangingBars);
			   }
			   curScanRight = beamAz + beamWidth;
		   }
		   else
		   {
			   /*------------*/
			   /* Going Left */
			   /*------------*/
			   curScanRight = beamAz + beamWidth;
		 	   beamAz -= curScanRate * SimLibMajorFrameTime;
			   if (beamAz <= targetAz)
			   {
				   beamAz = targetAz;
              ClearFlagBit(ChangingBars);
			   }
			   curScanLeft = beamAz - beamWidth;
		   }
	   }
	   else
	   {
		   curScanRight = beamAz + beamWidth;
		   curScanLeft = beamAz - beamWidth;
		   if (scanDir > 0.0F)
		   {
			   curScanBottom = beamEl - beamWidth;
			   beamEl += curScanRate * SimLibMajorFrameTime;
			   curScanTop    = beamEl + beamWidth;
		   }
		   else
		   {
			   curScanTop    = beamEl + beamWidth;
			   beamEl -= curScanRate * SimLibMajorFrameTime;
			   curScanBottom = beamEl - beamWidth;
		   }

		   /*------------------*/
		   /* Off the top edge */
		   /*------------------*/
		   if (beamEl > elScan || beamEl + seekerElCenter > MAX_ANT_EL)
		   {
           SetFlagBit(ChangingBars);
			   targetAz = beamAz + barWidth;
			   beamEl = elScan;
			   scanDir = ScanRev;
			   curScanTop    = beamEl + beamWidth;
		   }
		   /*---------------------*/
		   /* Off the bottom edge */
		   /*---------------------*/
		   else if (beamEl < -elScan || beamEl + seekerElCenter < -MAX_ANT_EL)
		   {
           SetFlagBit(ChangingBars);
			   targetAz = beamAz + barWidth;
			   beamEl = -elScan;
			   scanDir = ScanFwd;
			   curScanBottom = beamEl - beamWidth;
		   }

		   /*--------------------*/
		   /* Off the right edge */
		   /*--------------------*/
		   if (targetAz > azScan)
			   targetAz = -azScan;
	   }
   }
   /*-------------------*/
   /* Horizontal Search */
   /*-------------------*/
   else
   {
      // Check the SAM target if needed
      if (IsSet(SAMingTarget))
      {
         if (lockedTarget)
         {
            // Firstly, is the beam over the target? if yes, return to search
            // else, go to the target
   	      theta  = lockedTarget->BaseData()->Pitch();
		      az = TargetAz (platform, lockedTarget->BaseData()->XPos(), lockedTarget->BaseData()->YPos());
		      el = TargetEl (platform, lockedTarget->BaseData()->XPos(), lockedTarget->BaseData()->YPos(),
               lockedTarget->BaseData()->ZPos());

            if (theta > MAX_ANT_EL)
               el -= (theta - MAX_ANT_EL);
            else if (theta < -MAX_ANT_EL)
               el += (-MAX_ANT_EL - theta);

            // target in beam ?
		      if ((az >= curScanLeft && az <= curScanRight &&
			        el <= curScanTop && el >= curScanBottom) || scanDir == ScanRev )
            {
               // Can See, or, have seen so head home
               scanDir = ScanRev;
               if (beamAz > azScan)
               {
   			      beamAz -= curScanRate * SimLibMajorFrameTime;
                  if (beamAz < azScan)
                     beamAz = azScan;
               }
               else if (beamAz < azScan)
               {
   			      beamAz += curScanRate * SimLibMajorFrameTime;
                  if (beamAz > azScan)
                     beamAz = azScan;
               }

               // And in elevation
               if (beamEl > elScan)
               {
   			      beamEl -= curScanRate * SimLibMajorFrameTime;
                  if (beamEl < elScan)
                     beamEl = elScan;
               }
               else if (beamEl < elScan)
               {
   			      beamEl += curScanRate * SimLibMajorFrameTime;
                  if (beamEl > elScan)
                     beamEl = elScan;
               }

               if (beamAz == azScan && beamEl == elScan)
               {
                  ClearFlagBit (SAMingTarget);
                  ClearFlagBit (ChangingBars);
                  scanDir = ScanFwd;
               }
            }
		      else
            {
               az -= seekerAzCenter;
               el -= seekerElCenter;
               // Can't See
               scanDir = ScanFwd;
               if (beamAz > az)
               {
   			      beamAz -= curScanRate * SimLibMajorFrameTime;
                  if (beamAz < az)
                     beamAz = az;
               }
               else if (beamAz < az)
               {
   			      beamAz += curScanRate * SimLibMajorFrameTime;
                  if (beamAz > az)
                     beamAz = az;
               }

               // And in elevation
               if (beamEl > el)
               {
   			      beamEl -= curScanRate * SimLibMajorFrameTime;
                  if (beamEl < el)
                     beamEl = el;
               }
               else if (beamEl < el)
               {
   			      beamEl += curScanRate * SimLibMajorFrameTime;
                  if (beamEl > el)
                     beamEl = el;
               }

               // Keep the beam legal
               if (beamAz + seekerAzCenter > MAX_ANT_EL)
               {
                  beamAz = MAX_ANT_EL - seekerAzCenter;
                  scanDir = ScanRev;
               }
               else if (beamAz + seekerAzCenter < -MAX_ANT_EL)
               {
                  beamAz = -MAX_ANT_EL - seekerAzCenter;
                  scanDir = ScanRev;
               }

               if (beamEl + seekerElCenter > MAX_ANT_EL)
               {
                  beamEl = MAX_ANT_EL - seekerElCenter;
                  scanDir = ScanRev;
               }
               else if (beamEl + seekerElCenter < -MAX_ANT_EL)
               {
                  beamEl = -MAX_ANT_EL - seekerElCenter;
                  scanDir = ScanRev;
               }
            }

            // Where are we really looking
	         curScanLeft   = beamAz - beamWidth;
	         curScanRight  = beamAz + beamWidth;
	         curScanTop    = beamEl + beamWidth;
	         curScanBottom = beamEl - beamWidth;

            // These get added back in later for all modes
//	         curScanLeft   -= seekerAzCenter;
//	         curScanRight  -= seekerAzCenter;
//	         curScanTop    -= seekerElCenter;
//	         curScanBottom -= seekerElCenter;
         }
         else
         {
            ClearFlagBit (SAMingTarget);
            ClearFlagBit (ChangingBars);
            scanDir = ScanFwd;
         }
      }
	   /*----------------*/
	   /* Inbetween bars */
	   /*----------------*/
	   else if (IsSet(ChangingBars))
	   {
		   curScanRight = beamAz + beamWidth;
		   curScanLeft = beamAz - beamWidth;
		   /*----------*/
		   /* Going Up */
		   /*----------*/
		   if (targetEl > beamEl)
		   {
			   curScanBottom = beamEl - beamWidth;
			   beamEl += curScanRate * SimLibMajorFrameTime;
			   if (beamEl >= targetEl)
			   {
				   beamEl = targetEl;
              ClearFlagBit(ChangingBars);
			   }
			   curScanTop = beamEl + beamWidth;
		   }
		   /*------------*/
		   /* Going Down */
		   /*------------*/
		   else
		   {
			   curScanTop = beamEl + beamWidth;
			   beamEl -= curScanRate * SimLibMajorFrameTime;
			   if (beamEl <= targetEl)
			   {
				   beamEl = targetEl;
              ClearFlagBit(ChangingBars);
			   }
			   curScanBottom = beamEl - beamWidth;
		   }
	   }
	   else
	   {
		   curScanTop = beamEl + beamWidth;
		   curScanBottom = beamEl - beamWidth;

		   if (scanDir > 0.0F)
		   {
			   curScanLeft = beamAz + beamWidth;
			   beamAz += curScanRate * SimLibMajorFrameTime;
			   curScanRight = beamAz + beamWidth;
		   }
		   else
		   {
			   curScanRight = beamAz - beamWidth;
			   beamAz -= curScanRate * SimLibMajorFrameTime;
			   curScanLeft = beamAz - beamWidth;
		   }

		   /*------------*/
		   /* Right Edge */
		   /*------------*/
		   if (scanDir == ScanFwd &&
			   (beamAz > azScan || beamAz + seekerAzCenter > MAX_ANT_EL))
		   {
#if 0	// This will be nice but is a bit broken inside GMComposit.cpp  SCR 8/14/98
				// In GM DBS modes, the beam always sweeps from left to right
				if ((mode==GM) && ((flags & DBS1) || (flags&DBS2))) {
					beamAz = max (-azScan, -MAX_ANT_EL + seekerAzCenter);
				} else 
#endif
				{
					SetFlagBit(ChangingBars);
					targetEl = beamEl - barWidth;
					beamAz = min (azScan, MAX_ANT_EL - seekerAzCenter);
					scanDir = ScanRev;
					curScanRight = beamAz + beamWidth;
				}
		   }
		   /*-----------*/
		   /* Left Edge */
		   /*-----------*/
		   else if (scanDir == ScanRev &&
			   (beamAz < -azScan || beamAz + seekerAzCenter < -MAX_ANT_EL))
		   {
           SetFlagBit(ChangingBars);
			   targetEl = beamEl - barWidth;
			   beamAz = max (-azScan, -MAX_ANT_EL + seekerAzCenter);
			   scanDir = ScanFwd;
			   curScanRight = beamAz + beamWidth;
		   }

		   /*----------------*/
		   /* Off the bottom */
		   /*----------------*/
		   if (targetEl < -elScan)
         {
			   targetEl = elScan;
            if (mode == SAM)
            {
               SetFlagBit (SAMingTarget);
               scanDir = ScanFwd;
            }
         }
	   }
   }

	curScanLeft += seekerAzCenter;
	curScanRight += seekerAzCenter;
	curScanTop += seekerElCenter;
	curScanBottom += seekerElCenter;
}

int RadarDopplerClass::LookingAtObject (SimObjectType* target)
{
int retval;
float az, el, theta;
SimObjectLocalData* targetData = target->localData;

	if (!IsSet(SpaceStabalized))
	{
		if (targetData->az >= curScanLeft && targetData->az <= curScanRight &&
			targetData->el <= curScanTop && targetData->el >= curScanBottom)
			retval = TRUE;
		else
			retval = FALSE;
	}
	else
	{
   	theta  = target->BaseData()->Pitch();
		az = TargetAz (platform, target->BaseData()->XPos(), target->BaseData()->YPos());
		el = TargetEl (platform, target->BaseData()->XPos(), target->BaseData()->YPos(),
         target->BaseData()->ZPos());

      if (theta > MAX_ANT_EL)
         el -= (theta - MAX_ANT_EL);
      else if (theta < -MAX_ANT_EL)
         el += (-MAX_ANT_EL - theta);

		if (az >= curScanLeft && az <= curScanRight &&
			el <= curScanTop && el >= curScanBottom)
			retval = TRUE;
		else
			retval = FALSE;
	}

	return (retval);
}
