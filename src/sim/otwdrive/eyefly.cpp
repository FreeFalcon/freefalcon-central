#include "stdhdr.h"
#include "Graphics\Include\DrawBSP.h"
#include "Graphics\Include\RenderOW.h"
#include "Graphics\Include\RViewPnt.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "simfiltr.h"
#include "simio.h"
#include "simbase.h"
#include "falclist.h"
#include "simmath.h"

extern int keyboardPickleOverride;
extern int keyboardTriggerOverride;

void DecomposeMatrix (Trotation* matrix, float* pitch, float* roll, float* yaw);
SimBaseClass* eyeFlyTgt = NULL;

void OTWDriverClass::SetEyeFlyCameraPosition(float dT)
{
float e1dot, e2dot, e3dot, e4dot;
float enorm;
float p, q, r, throttle;
float psi, theta, phi;
float tmpX, tmpY, tmpZ;

   if (lastotwPlatform)
   {
      InsertObjectIntoDrawList (lastotwPlatform.get());
   }

   ownshipPos.x = flyingEye->XPos();
   ownshipPos.y = flyingEye->YPos();
   ownshipPos.z = flyingEye->ZPos();

	focusPoint.x = ownshipPos.x;
	focusPoint.y = ownshipPos.y;
	focusPoint.z = ownshipPos.z;


	//all joystick button functionality is now in sijoy
	//if (IO.ReadDigital(0))
   if (keyboardTriggerOverride) //THW 2003-11-11 fixed
   {
	  p = Math.DeadBand(IO.ReadAnalog(AXIS_ROLL), -0.05F, 0.05F) * (1.0F / 0.95F); 	// Retro 31Dec2003
      p = max ( min (p, 1.0F), -1.0F) * dT * 1.0f;

	  q = Math.DeadBand(IO.ReadAnalog(AXIS_PITCH), -0.05F, 0.05F) * (1.0F / 0.95F); 		// Retro 31Dec2003
      q = max ( min (q, 1.0F), -1.0F) * dT * 1.0f;

	  r = -Math.DeadBand(-IO.ReadAnalog(AXIS_YAW), -0.05F, 0.05F) * (1.0F / 0.95F); 	// Retro 31Dec2003
      r = max ( min (r, 1.0F), -1.0F) * dT * 1.0f;

	  if (IO.AnalogIsUsed (AXIS_THROTTLE)) 	// Retro 31Dec2003
      {
         //throttle = (1.0F - Math.DeadBand(IO.ReadAnalog(2), -0.03F, 0.03F)) * 0.5F;
		 throttle = 0.66666F * IO.ReadAnalog(AXIS_THROTTLE); 	// Retro 31Dec2003
         throttle *= throttle;
      }
      else
         throttle = 0.25F;

      //if (IO.ReadDigital(1))
	  if (keyboardPickleOverride) //THW 2003-11-11 fixed
		  throttle *= -1.0F;
   }
   else
   {
      p = q = r = throttle = 0.0F;
   }

   /*-----------------------------------*/
   /* quaternion differential equations */
   /*-----------------------------------*/
   e1dot = (-e4*p - e3*q - e2*r)*0.5F;
   e2dot = (-e3*p + e4*q + e1*r)*0.5F;
   e3dot = ( e2*p + e1*q - e4*r)*0.5F;
   e4dot = ( e1*p - e2*q + e3*r)*0.5F;

   /*-----------------------*/
   /* integrate quaternions */
   /*-----------------------*/
   e1 += e1dot;
   e2 += e2dot;
   e3 += e3dot;
   e4 += e4dot;

   /*--------------------------*/
   /* quaternion normalization */
   /*--------------------------*/
   enorm = (float)sqrt(e1*e1 + e2*e2 + e3*e3 + e4*e4);
   e1 /= enorm;
   e2 /= enorm;
   e3 /= enorm;
   e4 /= enorm;

   /*-------------------*/
   /* direction cosines */
   /*-------------------*/
   cameraRot.M11 = e1*e1 - e2*e2 - e3*e3 + e4*e4;
   cameraRot.M21 = 2.0F*(e3*e4 + e1*e2);
   cameraRot.M31 = 2.0F*(e2*e4 - e1*e3);

   cameraRot.M12 = 2.0F*(e3*e4 - e1*e2);
   cameraRot.M22 = e1*e1 - e2*e2 + e3*e3 - e4*e4;
   cameraRot.M32 = 2.0F*(e2*e3 + e4*e1);

   cameraRot.M13 = 2.0F*(e1*e3 + e2*e4);
   cameraRot.M23 = 2.0F*(e2*e3 - e1*e4);
   cameraRot.M33 = e1*e1 + e2*e2 - e3*e3 - e4*e4;

   tmpX = flyingEye->XPos() + throttle * 20000.0F * cameraRot.M11 * dT;
	tmpY = flyingEye->YPos() + throttle * 20000.0F * cameraRot.M21 * dT;
	tmpZ = flyingEye->ZPos() + throttle * 20000.0F * cameraRot.M31 * dT;
   flyingEye->SetPosition (tmpX, tmpY, tmpZ);

   DecomposeMatrix (&cameraRot, &theta, &phi, &psi);
   flyingEye->SetYPR(psi, theta, phi);
   FindNearestBuilding();
}

void OTWDriverClass::FindNearestBuilding(void)
{
	Tpoint at;
	Tpoint intersect;
	SimBaseClass* testFeature;
	char labelStr[40];
	float radius;

	if (eyeFlyTgt)
	{
		eyeFlyTgt->drawPointer->SetLabel("", 0xff00ff00);
		eyeFlyTgt = NULL;
	}

	renderer->GetAt(&at);
	if (viewPoint->GroundIntersection (&at, &intersect))
	{
		// Find a building
		VuListIterator featureWalker (SimDriver.featureList);
		testFeature = (SimBaseClass*)featureWalker.GetFirst();
		while (testFeature)
		{
			if (testFeature->drawPointer)
				radius = testFeature->drawPointer->Radius();
			else
				radius = 30.0F;
			if (fabs (intersect.x - testFeature->XPos()) < radius &&
				fabs (intersect.y - testFeature->YPos()) < radius)
			{
				eyeFlyTgt = testFeature;
				break;
			}
			testFeature = (SimBaseClass*)featureWalker.GetNext();
		}
	}

	if (eyeFlyTgt)
	{
		sprintf (labelStr, "State %d\n", eyeFlyTgt->Status() & VIS_TYPE_MASK);
		ShiAssert (strlen(labelStr) < 40);
		eyeFlyTgt->drawPointer->SetLabel(labelStr, 0xff00ff00);
	}
}
