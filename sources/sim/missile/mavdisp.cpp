#include "Graphics/Include/TViewPnt.h"
#include "Graphics/Include/renderir.h"
#include "Graphics/Include/rendertv.h"
#include "Graphics/Include/RViewPnt.h"
#include "Graphics/Include/Mono2D.h"
#include "Graphics/Include/DrawBSP.h"
#include "stdhdr.h"
#include "Classtbl.h"
#include "missile.h"
#include "otwdrive.h"
#include "mavdisp.h"
#include "simbase.h"
#include "Entity.h"
#include "sensclas.h"
#include "sms.h"
#include "aircrft.h"
#include "FalcLib/include/dispopts.h"

#define LOCK_RING_MAX_SIZE     0.65F  //JPG 21 Jan 04 - was 0.5F
#define LOCK_RING_MIN_SIZE     0.25F
#define LOCK_RING_TICK_SIZE    0.075F

extern bool g_bGreyMFD;
extern bool g_bGreyScaleMFD;
extern bool bNVGmode;

extern bool g_bRealisticAvionics;
extern float g_fMavEXPLevel;					//Wombat778 9-27-2003
extern float g_fMavFOVLevel;					//Wombat778 9-27-2003

MaverickDisplayClass::MaverickDisplayClass(SimMoverClass* newPlatform) : MissileDisplayClass (newPlatform)
{
	Falcon4EntityClassType* classPtr;

	onMainScreen = FALSE;
	seesTarget = FALSE;
	seekerAz = 0.0F;
	seekerEl = 0.0F;
	// JB 010120
	//curFOV   = 5.0F * DTR;
	//curFOV   = 6.0F * DTR;

	// RV - Biker - New FOV data from missile FMs
	//curFOV = g_fMavFOVLevel * DTR;						//Wombat778 9-28-2003
	if ((MissileClass*)platform && ((MissileClass*)platform)->GetFOVLevel() > 0)
		curFOV = 12.0f/((MissileClass*)platform)->GetFOVLevel() * DTR;
	else
		curFOV = g_fMavFOVLevel * DTR;
   
  	 // JB 010120
   toggleFOV = FALSE;
   trackStatus = NoTarget;

   classPtr = (Falcon4EntityClassType*)platform->EntityType();

   if (classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AGM65A ||
         classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AGM65B)
   {
      displayType = AGM65_TV;
   }
   else if (classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AGM65D ||
      classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AGM65G)
   {
      displayType = AGM65_IR;
   }
// 2000-09-20 S.G. SO WE CAN FLY ANY AIRCRAFT?!?
   else
      displayType = AGM65_TV;

   F4Assert (displayType != NoDisplay);
}

void MaverickDisplayClass::DisplayInit (ImageBuffer* image)
{
	if (!g_bGreyScaleMFD)
		g_bGreyMFD = false;

	switch (displayType)
	{
	  case AGM65_IR:
		privateDisplay =  new RenderIR;
		((RenderIR*)privateDisplay)->Setup (image, OTWDriver.GetViewpoint());
		break;

	  case AGM65_TV:
		privateDisplay =  new RenderTV;
		((RenderTV*)privateDisplay)->Setup (image, OTWDriver.GetViewpoint());
		break;
	}
	SetReady(TRUE);
    
	if ((g_bGreyMFD) && (!bNVGmode))
		privateDisplay->SetColor(GetMfdColor(MFD_WHITE));
	else
		privateDisplay->SetColor (0xff00ff00);

	//privateDisplay->SetColor (0xff00ff00);
	((Render3D*)privateDisplay)->SetFOV(curFOV);
}

void MaverickDisplayClass::Display (VirtualDisplay* newDisplay)
{
   display = newDisplay;

   // SCR  9/1/98  Is it every legal to have viewPoint NULL???
   ShiAssert( viewPoint );	// If we don't hit this after a while, get rid of onMainScreen...
   if (viewPoint)
      onMainScreen = TRUE;
   else
      onMainScreen = FALSE;

		if ((g_bGreyMFD) && (!bNVGmode))
			display->SetColor(GetMfdColor(MFD_WHITE));
		else
			display->SetColor(GetMfdColor(MFD_GREEN));

   if (IsReady())
      DrawDisplay();
}

void MaverickDisplayClass::DrawDisplay(void)
{
	float totalAngle;
	int tmpColor = display->Color();

	// RV - Biker - New FOV data from missile FMs
	float ZoomMin;
	float ZoomMax;

	if((MissileClass*)platform && ((MissileClass*)platform)->GetEXPLevel() > 0 && ((MissileClass*)platform)->GetFOVLevel() > 0) {
		ZoomMin = ((MissileClass*)platform)->GetFOVLevel();
		ZoomMax = ((MissileClass*)platform)->GetEXPLevel();
	}
	else {
		// Values are inverse
		ZoomMin = 12.0f/g_fMavFOVLevel;
		ZoomMax = 12.0f/g_fMavEXPLevel;
	}
		
	if (onMainScreen && display->type == VirtualDisplay::DISPLAY_GENERAL) {
		display->EndDraw();
		if (toggleFOV)
		{
			toggleFOV = FALSE;
		 
			/*if (curFOV > 3.0F * DTR)						//Wombat778 9-27-2003 Removed and replaced with values for EXP and FOV from cfg file
			// JB 010120 Fixed Mav FOV
            //	curFOV = 0.5F * DTR;
				curFOV = 3.0F * DTR;
			// JB 010120
			else
				curFOV = 6.0F * DTR;
			*/

			// RV - Biker - New FOV calculation (decrease curFOV -> increase zoom)
			//if (curFOV > g_fMavEXPLevel * DTR)				//Wombat778 9-28-2003 (reversed EXP and FOV...DOH!)
			//	 curFOV = g_fMavEXPLevel * DTR;
			//else
			//	 curFOV = g_fMavFOVLevel * DTR;

			if (curFOV > 12.0f/ZoomMax * DTR)
				curFOV = 12.0f/ZoomMax * DTR;
			else
				curFOV = 12.0f/ZoomMin * DTR;

         ((Render3D*)privateDisplay)->SetFOV(curFOV);
      }
      ShiAssert(platform->IsMissile());
      if (((MissileClass*)platform)->parent->IsAirplane() && ((AircraftClass*)((MissileClass*)platform)->parent.get())->Sms->MasterArm() != SMSBaseClass::Safe)
         DrawTerrain();
      display->StartDraw();
   }

		if ((g_bGreyMFD) && (!bNVGmode))
			display->SetColor(GetMfdColor(MFD_WHITE));
		else
			display->SetColor (tmpColor);
   display->Line (0.0F, -1.0F, 0.0F, -0.03F);
   display->Line (0.0F,  1.0F, 0.0F,  0.03F);
   display->Line (-1.0F, 0.0F, -0.03F, 0.0F);
   display->Line ( 1.0F, 0.0F,  0.03F, 0.0F);

   if(!g_bRealisticAvionics)
   {
	   display->Line (-0.1F, -0.2F, 0.1F, -0.2F);
	   display->Line (-0.1F, -0.4F, 0.1F, -0.4F);
	   display->Line (-0.1F, -0.6F, 0.1F, -0.6F);
   }
   else
   {
	   display->Line (-0.08F, -0.167F, 0.08F, -0.167F);
	   display->Line (-0.08F, -0.333F, 0.08F, -0.333F);
	   display->Line (-0.08F, -0.5F, 0.08F, -0.5F);
   }
   if (IsLocked())
   {
	   //MI
	   if(!g_bRealisticAvionics)
	   {
		   display->Line (-LOCK_RING_MIN_SIZE, -LOCK_RING_MIN_SIZE,
			   -(LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE), -LOCK_RING_MIN_SIZE);
		   display->Line (-LOCK_RING_MIN_SIZE, -LOCK_RING_MIN_SIZE,
			   -LOCK_RING_MIN_SIZE, -(LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE));
		   display->Line ( LOCK_RING_MIN_SIZE,  LOCK_RING_MIN_SIZE,
			   (LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE),  LOCK_RING_MIN_SIZE);
		   display->Line ( LOCK_RING_MIN_SIZE,  LOCK_RING_MIN_SIZE, 
			   LOCK_RING_MIN_SIZE,  (LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE));
		   display->Line (-LOCK_RING_MIN_SIZE,  LOCK_RING_MIN_SIZE,
			   -(LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE),  LOCK_RING_MIN_SIZE);
		   display->Line (-LOCK_RING_MIN_SIZE,  LOCK_RING_MIN_SIZE,
			   -LOCK_RING_MIN_SIZE,  (LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE));
		   display->Line ( LOCK_RING_MIN_SIZE, -LOCK_RING_MIN_SIZE,
			   (LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE), -LOCK_RING_MIN_SIZE);
		   display->Line ( LOCK_RING_MIN_SIZE, -LOCK_RING_MIN_SIZE,
			   LOCK_RING_MIN_SIZE, -(LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE));
	   }
	   else
	   {
			if ((g_bGreyMFD) && (!bNVGmode))
				display->SetColor(GetMfdColor(MFD_WHITE));
			else
				display->SetColor(GetMfdColor(MFD_GREEN));
			// RV - Biker - Make FOV switching this dynamic
			//if(curFOV > 3.0f * DTR)
			if(curFOV > 12.0f/(ZoomMax-(ZoomMax-ZoomMin)/2.0f) * DTR)
				DrawFOV();

		   //MI 02/02/02 appearently not here in real
		   /*const static float size = 0.08F;
		   display->Line (-size, -size, -size, size);
		   display->Line (-size, size, size, size);
		   display->Line (size, size, size, -size);
		   display->Line (size, -size, -size, -size);*/

	   }
   }
   else if (IsDetected())
   {
	   //MI doesn't do this in real
	   if(!g_bRealisticAvionics)
	   {
		   unsigned long tmp = vuxRealTime;
		   float offset = (float)(tmp & 0x3FF) / 0x400;

		   if (tmp & 0x400)
			   offset = 1.0F - offset;
		   offset *= LOCK_RING_MAX_SIZE - LOCK_RING_MIN_SIZE;

		   display->Line (-(LOCK_RING_MIN_SIZE + offset), -(LOCK_RING_MIN_SIZE + offset),
			   -((LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE) + offset), -(LOCK_RING_MIN_SIZE + offset));
		   display->Line (-(LOCK_RING_MIN_SIZE + offset), -(LOCK_RING_MIN_SIZE + offset),
			   -(LOCK_RING_MIN_SIZE + offset), -((LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE) + offset));
		   display->Line (  LOCK_RING_MIN_SIZE + offset,    LOCK_RING_MIN_SIZE + offset,
			   (LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE) + offset,    LOCK_RING_MIN_SIZE + offset);
		   display->Line (  LOCK_RING_MIN_SIZE + offset,    LOCK_RING_MIN_SIZE + offset,
			   LOCK_RING_MIN_SIZE + offset,    (LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE) + offset);
		   display->Line (-(LOCK_RING_MIN_SIZE + offset),   LOCK_RING_MIN_SIZE + offset,
			   -((LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE) + offset),   LOCK_RING_MIN_SIZE + offset);
		   display->Line (-(LOCK_RING_MIN_SIZE + offset),   LOCK_RING_MIN_SIZE + offset,
			   -(LOCK_RING_MIN_SIZE + offset),   (LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE) + offset);
		   display->Line (  LOCK_RING_MIN_SIZE + offset,  -(LOCK_RING_MIN_SIZE + offset),
			   (LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE) + offset,  -(LOCK_RING_MIN_SIZE + offset));
		   display->Line (  LOCK_RING_MIN_SIZE + offset,  -(LOCK_RING_MIN_SIZE + offset),
			   LOCK_RING_MIN_SIZE + offset,  -((LOCK_RING_MIN_SIZE - LOCK_RING_TICK_SIZE) + offset));
	   }
	   else {
			// RV - Biker - Make FOV switching this dynamic
			//if(curFOV > 3.0f * DTR)
			if ((g_bGreyMFD) && (!bNVGmode))
				display->SetColor(GetMfdColor(MFD_WHITE));
			else
				display->SetColor(GetMfdColor(MFD_GREEN));
			if(curFOV > 12.0f/(ZoomMax-(ZoomMax-ZoomMin)/2.0f) * DTR)
				DrawFOV();
		}
   }
   else
   {
	   if(!g_bRealisticAvionics)
	   {

		   display->Line (-LOCK_RING_MAX_SIZE, -LOCK_RING_MAX_SIZE,
			   -(LOCK_RING_MAX_SIZE - LOCK_RING_TICK_SIZE), -LOCK_RING_MAX_SIZE);
		   display->Line (-LOCK_RING_MAX_SIZE, -LOCK_RING_MAX_SIZE,
			   -LOCK_RING_MAX_SIZE, -(LOCK_RING_MAX_SIZE - LOCK_RING_TICK_SIZE));
		   display->Line ( LOCK_RING_MAX_SIZE,  LOCK_RING_MAX_SIZE,
			   (LOCK_RING_MAX_SIZE - LOCK_RING_TICK_SIZE),  LOCK_RING_MAX_SIZE);
		   display->Line ( LOCK_RING_MAX_SIZE,  LOCK_RING_MAX_SIZE,
			   LOCK_RING_MAX_SIZE,  (LOCK_RING_MAX_SIZE - LOCK_RING_TICK_SIZE));
		   display->Line (-LOCK_RING_MAX_SIZE,  LOCK_RING_MAX_SIZE,
			   -(LOCK_RING_MAX_SIZE - LOCK_RING_TICK_SIZE),  LOCK_RING_MAX_SIZE);
		   display->Line (-LOCK_RING_MAX_SIZE,  LOCK_RING_MAX_SIZE,
			   -LOCK_RING_MAX_SIZE,  (LOCK_RING_MAX_SIZE - LOCK_RING_TICK_SIZE));
		   display->Line ( LOCK_RING_MAX_SIZE, -LOCK_RING_MAX_SIZE,
			   (LOCK_RING_MAX_SIZE - LOCK_RING_TICK_SIZE), -LOCK_RING_MAX_SIZE);
		   display->Line ( LOCK_RING_MAX_SIZE, -LOCK_RING_MAX_SIZE,
			   LOCK_RING_MAX_SIZE, -(LOCK_RING_MAX_SIZE - LOCK_RING_TICK_SIZE));
	   }
	   else {
			if ((g_bGreyMFD) && (!bNVGmode))
				display->SetColor(GetMfdColor(MFD_WHITE));
			else
				display->SetColor(GetMfdColor(MFD_GREEN));
			// RV - Biker - Make FOV switching this dynamic
			//if(curFOV > 3.0f * DTR)
			if(curFOV > 12.0f/(ZoomMax-(ZoomMax-ZoomMin)/2.0f) * DTR)
				DrawFOV();
		}
   }

 	// FRB - B&W display
	if ((g_bGreyMFD) && (!bNVGmode))
		display->SetColor(GetMfdColor(MFD_WHITE));
	else
    display->SetColor(GetMfdColor(MFD_GREEN));

  if (!IsSOI())
   {
	   //MI
	   if(!g_bRealisticAvionics)
		   display->TextCenter(0.0F, 0.4F, "NOT SOI");
	   else
		   display->TextCenter(0.0F, 0.7F, "NOT SOI");
   }
   else {
			if ((g_bGreyMFD) && (!bNVGmode))
				display->SetColor(GetMfdColor(MFD_WHITE));
			else
				display->SetColor(GetMfdColor(MFD_GREEN));
       DrawBorder(); // JPO SOI
   }

   totalAngle = (float)acos (cos (platform->sensorArray[0]->SeekerEl()) * cos (platform->sensorArray[0]->SeekerAz()));
   //MI
   if(!g_bRealisticAvionics)
   {
	   if ((totalAngle < 30.0F * DTR) || vuxRealTime & 0x200)
	   {
		  display->AdjustOriginInViewport (platform->sensorArray[0]->SeekerAz(), platform->sensorArray[0]->SeekerEl());
		  display->Line (0.0F,  0.2F,  0.0F, -0.2F);
		  display->Line (0.2F,  0.0F, -0.2F,  0.0F);
		  display->AdjustOriginInViewport (-platform->sensorArray[0]->SeekerAz(), -platform->sensorArray[0]->SeekerEl());
	   }
   }
   else
   {
	   if(totalAngle < 30.0F * DTR && IsLocked())
	   {
		   display->AdjustOriginInViewport(platform->sensorArray[0]->SeekerAz(), platform->sensorArray[0]->SeekerEl());
		   display->Line (0.0F,  0.15F,  0.0F, -0.15F);
		   display->Line (0.15F,  0.0F, -0.15F,  0.0F);
		   display->AdjustOriginInViewport(-platform->sensorArray[0]->SeekerAz(), -platform->sensorArray[0]->SeekerEl());
	   }
	   else
	   {
		   if(vuxRealTime & 0x100)
		   {
			   display->AdjustOriginInViewport(platform->sensorArray[0]->SeekerAz(), platform->sensorArray[0]->SeekerEl());
			   display->Line (0.0F,  0.15F,  0.0F, -0.15F);
			   display->Line (0.15F,  0.0F, -0.15F,  0.0F);
			   display->AdjustOriginInViewport(-platform->sensorArray[0]->SeekerAz(), -platform->sensorArray[0]->SeekerEl());
		   }
	   }
   }
}

void MaverickDisplayClass::DrawTerrain(void)
{
Trotation viewRotation;
Tpoint cameraPos;
float xOff, yOff, zOff;
float tgtX, tgtY, tgtZ;
float costha,sintha,cospsi,sinpsi, sinphi, cosphi;
float pitch, yaw, range;
RViewPoint* tmpView;

   // Find missile realtive pos;
   GetXYZ (&xOff, &yOff, &zOff);
   tmpView = OTWDriver.GetViewpoint();
   cameraPos.x = xOff - tmpView->X();
   cameraPos.y = yOff - tmpView->Y();
   cameraPos.z = zOff - tmpView->Z();

   // RV - Biker - Give some offset to be sure camera isn't located inside missile
   Tpoint tmpPos;
   Tpoint newPos;

   // RV - Biker - OK better check for bad data so then no CTD
   if (platform->drawPointer) {
		tmpPos.x = platform->drawPointer->Radius();
		tmpPos.y = 0.0f;
		tmpPos.z = 0.0f;

		MatrixMult( &(OTWDriver.ownshipRot), &tmpPos, &newPos );

   		cameraPos.x += newPos.x;
   		cameraPos.y += newPos.y;
   		cameraPos.z += newPos.z;
   }

   if (IsSOI())
   {
	  ShiAssert(platform->IsMissile());
      ((MissileClass*)platform)->GetTargetPosition(&tgtX, &tgtY, &tgtZ);
	  xOff = tgtX - xOff;
      yOff = tgtY - yOff;
      zOff = tgtZ - zOff;

      yaw = (float)atan2 (yOff,xOff);
      cospsi = (float)cos (yaw);
      sinpsi = (float)sin (yaw);

      range = (float)sqrt(xOff*xOff+yOff*yOff+.1f);
      pitch = (float)atan (-zOff/range);
      costha = (float)cos (pitch);
      sintha = (float)sin (pitch);

      SetYPR (yaw, pitch, 0.0F);
    
      cosphi = 1.0F;
      sinphi = 0.0F;
   }
   else
   {
      costha = (float)cos (platform->Pitch());
      sintha = (float)sin (platform->Pitch());
      cospsi = (float)cos (platform->Yaw());
      sinpsi = (float)sin (platform->Yaw());
      cosphi = (float)cos (platform->Roll());
      sinphi = (float)sin (platform->Roll());
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

   // SCR:  I'm pretty sure this test is unnecessary since the
   // funtions are all virtualized if necessary...  Since I
   // don't want to test this, I'll leave it for now...
	
   // RV - RED - ZBuffer Enabled
	((RenderTV*)display)->context.SetZBuffering(TRUE);
/*	if (displayType == AGM65_IR)
	{*/
		((RenderIR*)display)->StartDraw();
		((RenderIR*)display)->DrawScene(&cameraPos, &viewRotation);

		((RenderIR*)display)->context.FlushPolyLists();
		((RenderIR*)display)->PostSceneCloudOcclusion();
		((RenderIR*)display)->EndDraw();
/*	}
	else if (displayType == AGM65_TV)
	{
	  ((RenderTV*)display)->StartDraw();
	  ((RenderTV*)display)->DrawScene(&cameraPos, &viewRotation);

	  ((RenderTV*)display)->EndDraw();
	}*/
}

void MaverickDisplayClass::LockTarget(void)
{
    ShiAssert(platform->IsMissile());
   if (((MissileClass*)platform)->targetPtr)
      trackStatus = TargetLocked;
   else
      trackStatus = NoTarget;
}

void MaverickDisplayClass::DetectTarget(void)
{
    ShiAssert(platform->IsMissile());
   if (((MissileClass*)platform)->targetPtr)
      trackStatus = TargetDetected;
   else
      trackStatus = NoTarget;
}

void MaverickDisplayClass::DropTarget(void)
{
   trackStatus = NoTarget;
}

int MaverickDisplayClass::IsCentered (SimBaseClass* testObject)
{
Tpoint from;
Tpoint at;
Tpoint coll;
DrawableBSP* drawPointer = (DrawableBSP*)testObject->drawPointer;
int didSee = FALSE;

   if (drawPointer && display)
   {
      from.x = ((Render3D*)display)->X();
      from.y = ((Render3D*)display)->Y();
      from.z = ((Render3D*)display)->Z();
      ((Render3D*)display)->GetAt(&at);
      didSee = drawPointer->GetRayHit(&from, &at, &coll, 4);
   }

   return didSee;
}
//MI
void MaverickDisplayClass::DrawFOV(void)
{
	static const float value = LOCK_RING_MAX_SIZE - 0.16F;
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