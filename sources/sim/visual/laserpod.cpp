#include "stdhdr.h"
#include "object.h"
#include "laserpod.h"
#include "Graphics\Include\renderir.h"
#include "simmover.h"
#include "entity.h"
#include "simmath.h"
#include "otwdrive.h"
#include "msginc\LaserDesignateMsg.h"
#include "falcmesg.h"
#include "falcsess.h"
#include "sms.h"
#include "aircrft.h"
#include "simdrive.h"	//MI
#include "fcc.h"	//MI
#include "FalcLib\include\dispopts.h"
#include "airframe.h"

/* 2001-09-07 S.G. */ extern bool g_bRP5Comp;

#define LOCK_RING_MAX_SIZE     0.5F
#define LOCK_RING_MIN_SIZE     0.25F
#define LOCK_RING_TICK_SIZE    0.075F
//MI we only got 150°
//#define LGB_GIMBAL_MAX         (160.0F * DTR)
#define LGB_GIMBAL_MAX         (150.0F * DTR)
extern bool g_bRealisticAvionics;
extern bool g_bGreyMFD;
extern bool g_bGreyScaleMFD;
extern bool bNVGmode;

LaserPodClass::LaserPodClass (int idx, SimMoverClass* self) : VisualClass (idx, self)
{
   visualType = TARGETINGPOD;
   //MI docs say it's different
   if(!g_bRealisticAvionics)
	   curFOV = 10.0F * DTR;
   else
	   curFOV = 6.0F * DTR;
   hasTarget = NoTarget;

   //MI
   BHOT = TRUE;
   MenuMode = FALSE;
}


LaserPodClass::~LaserPodClass (void)
{
   DisplayExit();
}


//SimObjectType* LaserPodClass::Exec (SimObjectType* newTargetList)
SimObjectType* LaserPodClass::Exec (SimObjectType*)
{
	if (hasTarget) {
		return lockedTarget;
	} else {
		return NULL;
	}
}


void LaserPodClass::DisplayInit (ImageBuffer* image)
{
	 if (!g_bGreyScaleMFD)
		 g_bGreyMFD = false;
   privateDisplay =  new RenderIR;
   ((RenderTV*)privateDisplay)->Setup (image, OTWDriver.GetViewpoint());
	 if ((g_bGreyMFD) && (!bNVGmode))
		 privateDisplay->SetColor(GetMfdColor(MFD_WHITE));
	 else
		 privateDisplay->SetColor (0xff00ff00);
   ((RenderTV*)privateDisplay)->SetFOV(curFOV);
   tgtX = platform->XPos();
   tgtY = platform->YPos();
   tgtZ = platform->ZPos();
}


void LaserPodClass::ToggleFOV (void)
{
	//MI Doc's say it's different
	if(!g_bRealisticAvionics)
	{
		if (curFOV > 3.0F * DTR)
			// JB 010120 Fixed FOV
			//curFOV = 0.25F * DTR;
			curFOV = 3.0F * DTR;
		// JB 010120
		else
			curFOV = 10.0F * DTR;
	}
	else
	{
		if(curFOV < 3.0F * DTR && curFOV > 1.6F * DTR)
			curFOV = 0.85F * DTR;
		else if(curFOV > 3.0F * DTR)
			curFOV = 1.7F * DTR;
		else
			curFOV = 6.0F * DTR;
	}
	if (privateDisplay)
		((Render3D*)privateDisplay)->SetFOV(curFOV);
}


void LaserPodClass::Display (VirtualDisplay* newDisplay)
{
		AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
		display = newDisplay;
		// FRB - B&W display
		if (g_bGreyMFD && !bNVGmode)
			display->SetColor(0xffffffff);

		int tmpColor = display->Color();

   if (privateDisplay)
   {
      if (viewPoint && display->type == VirtualDisplay::DISPLAY_GENERAL)
      {
         display->EndDraw();
        if (platform->IsAirplane() && ((AircraftClass*)platform)->Sms->MasterArm() != SMSBaseClass::Safe)
         {
            DrawTerrain();
         }
         display->StartDraw();
      }
      // Reset color after terrain
      display->SetColor (tmpColor);

	  //MI looks different in real
	  if(!g_bRealisticAvionics)
      {
		  display->Line (0.0F, -1.0F, 0.0F, -0.1F);
		  display->Line (0.0F,  1.0F, 0.0F,  0.1F);      
		  display->Line (-1.0F, 0.0F, -0.1F, 0.0F);
		  display->Line ( 1.0F, 0.0F,  0.1F, 0.0F);
		  display->Line (-0.1F, -0.2F, 0.1F, -0.2F);
		  display->Line (-0.1F, -0.4F, 0.1F, -0.4F);
		  display->Line (-0.1F, -0.6F, 0.1F, -0.6F);
	  }
	  else
	  {
		  if(!MenuMode)
		  {
			  float begin = 0.1F;
			  float end = 0.3F;
			  display->Line(0.0F, -begin, 0.0F, -end);
			  display->Line(0.0F, begin, 0.0F, end);
			  display->Line(-begin, 0.0F, -end, 0.0F);
			  display->Line(begin, 0.0F, end, 0.0F);
		  }
	  }

      if (hasTarget == TargetLocked)
      {
		  //MI
		  if(!g_bRealisticAvionics)
		  {
			  display->Line (-LOCK_RING_MIN_SIZE, -LOCK_RING_MIN_SIZE, -LOCK_RING_MIN_SIZE,  LOCK_RING_MIN_SIZE);
			  display->Line (-LOCK_RING_MIN_SIZE, -LOCK_RING_MIN_SIZE,  LOCK_RING_MIN_SIZE, -LOCK_RING_MIN_SIZE);
			  display->Line ( LOCK_RING_MIN_SIZE,  LOCK_RING_MIN_SIZE, -LOCK_RING_MIN_SIZE,  LOCK_RING_MIN_SIZE);
			  display->Line ( LOCK_RING_MIN_SIZE,  LOCK_RING_MIN_SIZE,  LOCK_RING_MIN_SIZE, -LOCK_RING_MIN_SIZE);
		  }
		  else
		  {
				// FRB - B&W display
				if (g_bGreyMFD && !bNVGmode)
					display->SetColor(0xffffffff);
			  //only if not in narrow view
			  if(curFOV > 3.0F * DTR)
				  DrawFOV(display);
			  if(playerAC && playerAC->FCC && !playerAC->FCC
				  ->preDesignate && !IsLocked())
				  display->TextCenter(0.0F, -0.4F, "AREA");
		  }

      }
      else if (lockedTarget)
      {
      unsigned long tmp = vuxRealTime;
      float offset = (float)(tmp & 0x3FF) / 0x400;
   
         if (tmp & 0x400)
            offset = 1.0F - offset;
         offset *= LOCK_RING_MAX_SIZE - LOCK_RING_MIN_SIZE;
		
		 //MI
		 if(!g_bRealisticAvionics)
		 {
			 display->Line (-(LOCK_RING_MIN_SIZE + offset), -(LOCK_RING_MIN_SIZE + offset), -(LOCK_RING_MIN_SIZE + offset),   LOCK_RING_MIN_SIZE + offset);
			 display->Line (-(LOCK_RING_MIN_SIZE + offset), -(LOCK_RING_MIN_SIZE + offset),   LOCK_RING_MIN_SIZE + offset,  -(LOCK_RING_MIN_SIZE + offset));
			 display->Line (  LOCK_RING_MIN_SIZE + offset,    LOCK_RING_MIN_SIZE + offset,  -(LOCK_RING_MIN_SIZE + offset),   LOCK_RING_MIN_SIZE + offset);
			 display->Line (  LOCK_RING_MIN_SIZE + offset,    LOCK_RING_MIN_SIZE + offset,    LOCK_RING_MIN_SIZE + offset,  -(LOCK_RING_MIN_SIZE + offset));
		 }
		 else
		 {
			 //only if not in narrow view
			 if(curFOV > 3.0F * DTR)
				 DrawFOV(display);
			 if(playerAC && playerAC->FCC && !playerAC->FCC
				  ->preDesignate && !IsLocked())
				  display->TextCenter(0.0F, -0.4F, "AREA");
		 }
      }
      else
      {
		  //MI
		  if(!g_bRealisticAvionics)
		  {
			  display->Line (-LOCK_RING_MAX_SIZE, -LOCK_RING_MAX_SIZE, -LOCK_RING_MAX_SIZE,  LOCK_RING_MAX_SIZE);
			  display->Line (-LOCK_RING_MAX_SIZE, -LOCK_RING_MAX_SIZE,  LOCK_RING_MAX_SIZE, -LOCK_RING_MAX_SIZE);
			  display->Line ( LOCK_RING_MAX_SIZE,  LOCK_RING_MAX_SIZE, -LOCK_RING_MAX_SIZE,  LOCK_RING_MAX_SIZE);
			  display->Line ( LOCK_RING_MAX_SIZE,  LOCK_RING_MAX_SIZE,  LOCK_RING_MAX_SIZE, -LOCK_RING_MAX_SIZE);
		  }
		  else
		  {
				// FRB - B&W display
				if (g_bGreyMFD && !bNVGmode)
					display->SetColor(0xffffffff);
			  //only if not in narrow view
			  if(curFOV > 3.0F * DTR)
				  DrawFOV(display);
			  if(playerAC && playerAC->FCC && !playerAC->FCC
				  ->preDesignate && !IsLocked())
				  display->TextCenter(0.0F, -0.4F, "AREA");
		  }
      }

      if (!IsSOI())
      {
				if(g_bRealisticAvionics && !MenuMode)
				{
					// FRB - B&W display
					if (g_bGreyMFD && !bNVGmode)
						display->SetColor(0xffffffff);
					else
						display->SetColor(GetMfdColor(MFD_GREEN));
					//Not here in real
					//display->TextCenter(0.0F, 0.4F, "NOT SOI");
				}
				else if(!g_bRealisticAvionics)
				{
					display->SetColor(GetMfdColor(MFD_GREEN));
					display->TextCenter(0.0F, 0.4F, "NOT SOI");
				}
      }
      else
			{
				// FRB - B&W display
				if (g_bGreyMFD && !bNVGmode)
					display->SetColor(0xffffffff);
				else
					display->SetColor(GetMfdColor(MFD_GREEN));
				DrawBorder(); // JPO SOI
      }
	  
	  //MI not here in real, according to docs and MFD vids from Kosovo bombings
	  if(!g_bRealisticAvionics)
	  {
		  display->AdjustOriginInViewport (seekerAzCenter / LGB_GIMBAL_MAX, seekerElCenter / LGB_GIMBAL_MAX);
		  display->Line (0.0F,  0.2F,  0.0F, -0.2F);
		  display->Line (0.2F,  0.0F, -0.2F,  0.0F);
		  display->AdjustOriginInViewport (-seekerAzCenter / LGB_GIMBAL_MAX, -seekerElCenter / LGB_GIMBAL_MAX);
	  }
	  else	//but instead, there's a small square
	  {
		  if(!MenuMode)
		  {
				// FRB - B&W display
				if (g_bGreyMFD && !bNVGmode)
					display->SetColor(0xffffffff);
				else
					display->SetColor(GetMfdColor(MFD_GREEN));
			  static float size = 0.02F;
			  display->AdjustOriginInViewport (seekerAzCenter / LGB_GIMBAL_MAX, seekerElCenter / LGB_GIMBAL_MAX);
			  display->Tri(-size, size,-size, -size, size, size);
			  display->Tri(size, size, size, -size, -size, -size);
			  display->AdjustOriginInViewport (-seekerAzCenter / LGB_GIMBAL_MAX, -seekerElCenter / LGB_GIMBAL_MAX);
		  }
	  }
	  //MI
	  if(g_bRealisticAvionics)
	  {
		  if(playerAC && playerAC->FCC)
		  {
				// FRB - B&W display
				if (g_bGreyMFD && !bNVGmode)
					display->SetColor(0xffffffff);
				else
					display->SetColor(GetMfdColor(MFD_GREEN));
			  if(playerAC->FCC->designateCmd || IsLocked())
				  DrawBox(display);
		  }
	  }
   }
}


void LaserPodClass::DrawTerrain(void)
{
	Trotation viewRotation;

	// RV - Biker - We have data for Lantirn Camera so why not use it
	Tpoint cameraPos;

	Tpoint tmpPos;
	Tpoint newPos;

	tmpPos.x = ((AircraftClass*)platform)->af->GetLantirnCameraX();
	tmpPos.y = ((AircraftClass*)platform)->af->GetLantirnCameraY();
	tmpPos.z = ((AircraftClass*)platform)->af->GetLantirnCameraZ();

	MatrixMult( &(OTWDriver.ownshipRot), &tmpPos, &newPos );

	cameraPos.x = newPos.x;
   	cameraPos.y = newPos.y;
   	cameraPos.z = newPos.z;

	float costha,sintha,cospsi,sinpsi, sinphi, cosphi;
	float xOff, yOff, zOff;
	float range;

   if (IsSOI())
   {
      xOff = tgtX - platform->XPos();
      yOff = tgtY - platform->YPos();
      zOff = tgtZ - platform->ZPos();

      yaw = (float)atan2 (yOff,xOff);
      cospsi = (float)cos (yaw);
      sinpsi = (float)sin (yaw);

      range = (float)sqrt(xOff*xOff+yOff*yOff+0.1f);
      pitch = (float)atan (-zOff/range);
      costha = (float)cos (pitch);
      sintha = (float)sin (pitch);

      roll = 0.0F;
    
      cosphi = 1.0F;
      sinphi = 0.0F;
   }
   else
   {
      costha = (float)cos (pitch);
      sintha = (float)sin (pitch);
      cospsi = (float)cos (yaw);
      sinpsi = (float)sin (yaw);
      cosphi = (float)cos (roll);
      sinphi = (float)sin (roll);
   }

   viewRotation.M11 = cospsi*costha;
   viewRotation.M21 = sinpsi*costha;
   viewRotation.M31 = -sintha;

   viewRotation.M12 = -sinpsi*cosphi + cospsi*sintha*sinphi;
   viewRotation.M22 = cospsi*cosphi + sinpsi*sintha*sinphi;
   viewRotation.M32 = costha*sinphi;

   viewRotation.M13 = sinpsi*sinphi + cospsi*sintha*cosphi;
   viewRotation.M23 = -cospsi*sinphi + sinpsi*sintha*cosphi;
   viewRotation.M33 = costha*cosphi;

   ((RenderTV*)display)->StartDraw();
   ((RenderTV*)display)->DrawScene(&cameraPos, &viewRotation);

	//JAM 12Dec03 - ZBUFFERING OFF
	if(DisplayOptions.bZBuffering)
		((RenderTV*)display)->context.FlushPolyLists();

//   ((RenderTV*)display)->PostSceneCloudOcclusion();
   ((RenderTV*)display)->EndDraw();
}


void LaserPodClass::SetDesiredTarget(SimObjectType *newTarget)
{
	FalconLaserDesignateMsg* msg;
	
	if (newTarget == lockedTarget)
		return;
	
	// Undesignate the current target
	if (lockedTarget && lockedTarget->BaseData()->IsSim())
	{
		msg = new FalconLaserDesignateMsg(platform->Id(), FalconLocalGame);
		msg->dataBlock.source   = platform->Id();
		msg->dataBlock.target   = lockedTarget->BaseData()->Id();
		msg->dataBlock.state    = FALSE;
		FalconSendMessage (msg,TRUE);
	}
	
	if (newTarget && newTarget->BaseData()->IsSim() && CanSeeObject(newTarget) && CanDetectObject(newTarget))
	{
		SetSensorTarget( newTarget );

		// Designate it
		msg = new FalconLaserDesignateMsg(platform->Id(), FalconLocalGame);
		msg->dataBlock.source   = platform->Id();
		msg->dataBlock.target   = lockedTarget->BaseData()->Id();
		msg->dataBlock.state    = TRUE;
		FalconSendMessage (msg,TRUE);
	}
	else
	{
		ClearSensorTarget();
		hasTarget = NoTarget;
	}
}


int LaserPodClass::SetDesiredSeekerPos (float* az, float* el)
{
int retval = FALSE;
float totalAngle, tmp;

	// Gimbal limit the seeker
#if 0 
//MI original code
	// Looking down or left your total angle is limited to 160 Degrees
   if (*el < 5.0F * DTR || *az < 5.0F * DTR)
   {
      tmp = (float)(cos (*az) * cos (*el));
      totalAngle = (float)atan2(sqrt(1-tmp*tmp),tmp);

      if (totalAngle > LGB_GIMBAL_MAX)
      {
         retval = TRUE;
      }
   }
   else //looking up and right
   {
      if (*az > 5.0F * DTR)
      {
         *az = 5.0F * DTR;
         retval = TRUE;
      }

      if (*el > 5.0F * DTR)
      {
         *el = 5.0F * DTR;
         retval = TRUE;
      }
   }
#endif
// 2001-09-07 ADDED BY S.G. RP5 DEALS WITH THE LIMIT DIFFERENTLY
   if (!g_bRP5Comp) {
	   // Looking down or left/right your total angle is limited to 150 Degrees
	   if (*el < 35.0F * DTR || *az > 5.0F * DTR)
	   {
		  tmp = (float)(cos (*az) * cos (*el));
		  totalAngle = (float)atan2(sqrt(1-tmp*tmp),tmp);

		  if (totalAngle > LGB_GIMBAL_MAX)
		  {
			 retval = TRUE;
		  }
	   }
		//up --> down is covered with the check above
	   if (*el > 35.0F * DTR) //35 degree up
	   {
		   *el = 35.0F * DTR;
		   retval = TRUE;
	   }
   }
   else {
// 2001-04-17 REWRITTEN BY S.G. WHAT IT SHOULD HAVE BEEN
		// az goes from -PI to PI (-180 to 180) and el goes from -PI/2 to PI/2 (-90 to 90).
		// If el needs to go lower than -90 or higher than 90, az is moved to the next quadrant so el is readjusted accordingly.
		// First for the front section of the plane (right side will have better elevation after 20 degrees)
		if (*az >= -90.0f * DTR && *az <= 20.0f * DTR) {
			if (*el > 20.0f * DTR) {
				*el = 20.0f * DTR;
				 retval = TRUE;
			}
		}
		// Then the right side of the plane
		else if (*az > 20.0f * DTR && *az < 120.0f * DTR) {
			if (*el > 60.0f * DTR) {
				*el = 60.0f * DTR;
				 retval = TRUE;
			}
		}
		// Now the extended back side (from -90 to +120, what is remaining)
		else {
			// If we're limited on the azimuth, simulate the pod looking back and limited at -150 degrees elevation)
			if (*az > 150.0f * DTR || *az < -150.0f * DTR) {
				if (*el > -30.f * DTR) {
					*el = -30.0f * DTR;
					 retval = TRUE;
				}
			}

			// From -90 to +120, can't see higher than 0 degree (blocked by the wings/frame)
			if (*el > 0.0f * DTR) {
				*el = 0.0f * DTR;
				 retval = TRUE;
			}

		}
   }
   // Send the new values to the base class
   if (!retval)
      SetSeekerPos( *az, *el );

   // Tell the caller if he hit the limits
   return retval;
}


int LaserPodClass::LockTarget (void)
{
   if (lockedTarget)
      hasTarget = TargetLocked;
   else
      hasTarget = NoTarget;

   return hasTarget;
}


// Helper Function to find a Targeting Pod
SensorClass* FindLaserPod (SimMoverClass* theObject)
{
int i;
ShiAssert(theObject);
SensorClass* retval = NULL;

if (!theObject) return retval;//me123 ctd
   for (i=0; i<theObject->numSensors; i++)
   {
		 if (theObject->sensorArray && theObject->sensorArray[i] && theObject->sensorArray[i]->Type() == SensorClass::TargetingPod)
		 {
        retval = theObject->sensorArray[i];
        break;
		 }
     else if (theObject->sensorArray && theObject->sensorArray[i] && theObject->sensorArray[i]->Type() == SensorClass::Visual)
     {
         if (((VisualClass*)theObject->sensorArray[i])->VisualType() == VisualClass::TARGETINGPOD)
         {
            retval = theObject->sensorArray[i];
            break;
         }
     }
   }
   return retval;
}
//MI
void LaserPodClass::DrawBox(VirtualDisplay* display)
{
	if(MenuMode)
		return;
	float len = 0.1F;
	display->Line(-len, len, len, len);
	display->Line(len, len, len, -len);
	display->Line(len, -len, -len, -len);
	display->Line(-len, -len, -len, len);
	if(hasTarget == TargetLocked)
		display->TextCenter(0.0F, -0.4F, "POINT");		
}
void LaserPodClass::DrawFOV(VirtualDisplay* display)
{
	if(MenuMode)
		return;
	static const float value = LOCK_RING_MAX_SIZE - 0.2F;
	static const float Lenght = 0.07F;
	display->Line(value, -value, value, -value + Lenght);
	display->Line(-value, -value, -value, -value + Lenght);
	display->Line(value, value, value, value - Lenght);
	display->Line(-value, value, -value,  value - Lenght);
	display->Line(value, -value, value - Lenght, -value);
	display->Line(-value, -value, -value + Lenght, -value);
	display->Line(value, value, value - Lenght, value);
	display->Line(-value, value, -value + Lenght, value);
}
