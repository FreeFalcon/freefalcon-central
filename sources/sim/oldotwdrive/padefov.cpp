
// ------------------------------------------------------------------------------
// 
//	File: padefov.cpp
//
//	This file contains code that is specific to the Extended Field of View padlock
// scheme.  Note that the code in this file can call the general padlock sorting
// functions.  On Gilman's orders, I stripped out code relating to the Hawkeye
// padlock.  See otwdrive\padhawk.cpp for the original unmolested Hawkeye code.
//
// List of Contents:
//		OTWDriverClass::PadlockEFOV_Draw()
//		OTWDriverClass::PadlockEFOV_DrawBox()
//
// ------------------------------------------------------------------------------

#include "Graphics\Include\grtypes.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\drawpnt.h"
#include "Graphics\Include\renderow.h"
#include "vehicle.h"
#include "simmover.h"
#include "stdhdr.h"
#include "otwdrive.h"
#include "object.h"
#include "playerop.h"
#include "simdrive.h"
#include "aircrft.h"
#include "FalcLib\include\dispopts.h"

// ------------------------------------------------------------------------------
//
//	OTWDriverClass::PadlockEFOV_Draw()
//
//	Arguments:
//		NONE
//
//	Returns:
//		NONE
//
//	This function checks if the player has requested to step thru the list of
// objects. If so it calls Padlock_FindNextPriority() to step to the next
// padlock candidate.  Then the function searches the target list for the currently
// padlock object.  If we find it in the target list, we can just copy the ATA and
// DROLL values (the quick way), otherwise we have to calculate it (the slow way).
// If the values of ATA and DROLL tell use that the object is off screen, then we
// draw the EFOV padlock window.
// 
// ------------------------------------------------------------------------------

void OTWDriverClass::PadlockEFOV_Draw(void)
{
	SimObjectType*	visObj=NULL;
//	int				tmpTexLevel=0;
	int				tmpObjTex=0;
//	int				tmpShade=0;
	float			tmpFov=0.0F;
	float			viewLimit=0.0F;
	float			viewDelta=0.0F;
	float			efovBoxSize=0.0F;
	float			ata=0.0F;
	float			droll=0.0F;
	float			ataFrom=0.0F;
	float			az=0.0F;
	float			el=0.0F;
	BOOL			found = FALSE;

	// If the player is requesting a change of target,
	// find the the candidate with the next lowest priority
   if (tgtStep) {
      Padlock_FindNextPriority(FALSE);
   }

	// Get the head of the target list.
	visObj		= ((SimMoverClass*)otwPlatform)->targetList;

	// Walk the target list and search for the padlock priority object
   while(visObj != NULL && found == FALSE) {

		// If the padlock priority object is in the list safe some important angle values.
      if(visObj->BaseData() == mpPadlockPriorityObject) {
			ata		= visObj->localData->ata;
			ataFrom	= visObj->localData->ataFrom;
			droll		= visObj->localData->droll;
			az			= visObj->localData->az;
			el			= visObj->localData->el;
			found		= TRUE;
		}

      visObj	= visObj->next;
   }

	// If we have a padlock priority object, but didn't find it in the target list,
	// we'll have to calculate the angles ourselves.
	if(visObj == NULL && mpPadlockPriorityObject) {
		CalcRelValues ((SimBaseClass*) SimDriver.playerEntity,													
								mpPadlockPriorityObject,
								&az, &el, &ata, &ataFrom, &droll);
		found = TRUE;
	}

	// Calculate the limits of our padlock view
	tmpFov			= renderer->GetFOV();
	efovBoxSize		= 0.25F;
	viewLimit		= 0.75F * tmpFov * 0.5F;
	viewDelta		= (tmpFov * 0.5F) - viewLimit;

	// If I have something to draw and it's off screen
	if(found == TRUE && (ata > viewLimit + viewDelta * (float)sin(fabs(droll))))
	{
		if(PlayerOptions.GetPadlockMode() == PDRealistic) {
			renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
			Padlock_DrawSquares(FALSE);
		}

		// Save our current renderer settings
//		tmpTexLevel = renderer->GetTerrainTextureLevel();
		tmpObjTex	= renderer->GetObjectTextureState();
//		tmpShade    = TRUE;//renderer->GetSmoothShadingMode();

		// Get the renderer ready to draw the padlocked object
		renderer->SetFOV(25.0F * DTR);
//		renderer->SetTerrainTextureLevel( 0 );
		renderer->SetObjectTextureState( FALSE );
//		renderer->SetSmoothShadingMode( FALSE );
		// Draw the contents of the Padlock window
		if(mpPadlockPriorityObject && mpPadlockPriorityObject->IsSim()) {
			PadlockEFOV_DrawBox(mpPadlockPriorityObject, efovBoxSize, az, el, ata, ataFrom, droll);
		}

		// Restore the renderer's original settings
		renderer->SetFOV(tmpFov);
//		renderer->SetTerrainTextureLevel( tmpTexLevel );
		renderer->SetObjectTextureState( tmpObjTex );
//		renderer->SetSmoothShadingMode( tmpShade );
	}
	else if(found == TRUE) {
		renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
		Padlock_DrawSquares(TRUE);
	}

}

#if 0
// ------------------------------------------------------------------------------
//
//	OTWDriverClass::PadlockEFOV_EasyDraw()
//
//	Arguments:
//		NONE
//
//	Returns:
//		NONE
//
// This is essentially the original 
//	This function checks if the player has requested to step thru the list of
// objects. If so it calls Padlock_FindNextPriority() to step to the next
// padlock candidate.  Then the function searches the target list for the currently
// padlock object.  If we find it in the target list, we can just copy the ATA and
// DROLL values (the quick way), otherwise we have to calculate it (the slow way).
// If the values of ATA and DROLL tell use that the object is off screen, then we
// draw the EFOV padlock window.
// 
// ------------------------------------------------------------------------------

void OTWDriverClass::PadlockEFOV_EasyDraw(void)
{
	SimObjectType*	visObj;
	SimObjectType*	priorityObj;
//	int				tmpTexLevel;
	int				tmpObjTex;
//	int				tmpShade;
	float				tmpFov;
	float				viewLimit;
	float				viewDelta;
	float				efovBoxSize;

	visObj		= ((SimMoverClass*)otwPlatform)->targetList;
	priorityObj = NULL;

	tmpFov		= renderer->GetFOV();
//	tmpTexLevel = renderer->GetTerrainTextureLevel();
	tmpObjTex	= renderer->GetObjectTextureState();
//	tmpShade    = TRUE;//renderer->GetSmoothShadingMode();

   renderer->SetFOV(25.0F * DTR);
//   renderer->SetTerrainTextureLevel( 0 );
   renderer->SetObjectTextureState( FALSE );
//   renderer->SetSmoothShadingMode( FALSE );

   efovBoxSize = 0.25F;
   viewLimit	= 0.75F * tmpFov * 0.5F;
   viewDelta	= (tmpFov * 0.5F) - viewLimit;

   // Find a new target
   if (tgtStep) {
      Padlock_FindNextPriority(FALSE);
   }

   // Go through the list of objects and find priority object
   while (visObj) {
      if (visObj->BaseData() == mpPadlockPriorityObject) {
         priorityObj = visObj;
		}

      visObj	= visObj->next;
   }

   // If found the thing to draw
   if (priorityObj) {

      if (priorityObj->localData->ata >
            viewLimit + viewDelta * (float)sin(fabs(priorityObj->localData->droll)))
      {
         PadlockEFOV_DrawBox(priorityObj, efovBoxSize);
      }
   }
   else {
      tgtStep	= 1;
   }

   renderer->SetFOV(tmpFov);
//   renderer->SetTerrainTextureLevel( tmpTexLevel );
   renderer->SetObjectTextureState( tmpObjTex );
//   renderer->SetSmoothShadingMode( tmpShade );
}
#endif
//**//



// ------------------------------------------------------------------------------
//
//	OTWDriverClass::PadlockEFOV_DrawBox()
//
//	Arguments:
//		NONE
//
//	Returns:
//		NONE
//
//	This function handles all the steps necessary to render the padlocked object,
// display it into a viewport, and draw visual cues (arrow) for the play.
// ------------------------------------------------------------------------------

void OTWDriverClass::PadlockEFOV_DrawBox (SimBaseClass* base, float efovBoxSize, float az, float el, float ata, float ataFrom, float droll)
{
	char						tmpStr[32];
	float						xPos;
	float						yPos;
	float						viewLimit;
	float						viewDelta;
	Tpoint					simView;
	Tpoint					zeroPos = {0.0F, 0.0F, 0.0F};
	Trotation				viewRotation;
	Trotation				tilt;
	Trotation				view;
	DrawableBSP*			bcBSP;
	mlTrig					trig;
	BOOL						simLabelState;
	BOOL						campLabelState;
	float						prevFOV;
	float						prevLeft;
	float						prevRight;
	float						prevTop;
	float						prevBottom;


	// Save off the current renderer info
	renderer->GetViewport( &prevLeft, &prevTop, &prevRight, &prevBottom );
	prevFOV		= renderer->GetFOV();

	// Set position of the EFOV box
   xPos			= 0.0F;
   yPos			= -0.75F;

	// Calculate the edges of the screen
   viewLimit	= 0.75F * renderer->GetFOV() * 0.5F;
   viewDelta	= (renderer->GetFOV() * 0.5F) - viewLimit;

   // Draw a big arrow
   renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
   renderer->SetColor (0xff00ff00);
   renderer->AdjustRotationAboutOrigin(droll);
   renderer->Line (0.0F, 1.0F, 0.1F, 0.9F);
   renderer->Line (0.0F, 1.0F, -0.1F, 0.9F);
   renderer->Line (0.1F, 0.9F, -0.1F, 0.9F);
   renderer->ZeroRotationAboutOrigin();

   ObjectSetData (otwPlatform, &simView, &viewRotation);

   // Spin for azimuth
   tilt			= IMatrix;
   mlSinCos (&trig, az);

   tilt.M11		= trig.cos;
   tilt.M12		= -trig.sin;
   tilt.M21		= -tilt.M12;
   tilt.M22		= tilt.M11;

   MatrixMult (&viewRotation, &tilt, &view);
   memcpy (&viewRotation, &view, sizeof (view));

   // Look up or down as required
   tilt			= IMatrix;
   mlSinCos (&trig, el);

   tilt.M11		= trig.cos;
   tilt.M13		= trig.sin;
   tilt.M31		= -tilt.M13;
   tilt.M33		= tilt.M11;

   MatrixMult (&viewRotation, &tilt, &view);
   memcpy (&viewRotation, &view, sizeof (view));

   renderer->SetViewport (xPos - efovBoxSize, yPos + efovBoxSize, xPos + efovBoxSize, yPos - efovBoxSize);

	simLabelState	= DrawableBSP::drawLabels;
	campLabelState	= DrawablePoint::drawLabels;
   DrawableBSP::drawLabels = FALSE;
   DrawablePoint::drawLabels = FALSE;

   renderer->DrawScene(&zeroPos, &viewRotation);

  	//JAM 12Dec03 - ZBUFFERING OFF
	if(DisplayOptions.bZBuffering)
		renderer->context.FlushPolyLists();

//	renderer->PostSceneCloudOcclusion();

	DrawableBSP::drawLabels		= simLabelState;
	DrawablePoint::drawLabels	= campLabelState;

   renderer->SetColor (0xff00ff00);

   renderer->Line(-0.97F, -0.97F, -0.97F,  1.00F);
   renderer->Line(-0.97F, -0.97F,  1.00F, -0.97F);
   renderer->Line( 1.00F,  1.00F, -0.97F,  1.00F);
   renderer->Line( 1.00F,  1.00F,  1.00F, -0.97F);

   renderer->Line( 0.9F, -1.0F + 2.0F * ata / PI, 0.9F, -1.0F);
   renderer->Line( 0.85F, -1.0F + 2.0F * viewLimit / PI, 0.95F, -1.0F + 2.0F * viewLimit / PI);
   renderer->Line(-0.9F,  1.0F - 2.0F * ataFrom / PI, -0.9F, 1.0F);

   // Add the tag if needed
   if (PlayerOptions.NameTagsOn()) {
		tmpStr[0]	= 0;

		if((bcBSP	= (DrawableBSP *)base->drawPointer) != NULL) {
         sprintf (tmpStr, "%s", bcBSP->Label());
      }

		ShiAssert (strlen(tmpStr) < 32);
      renderer->TextCenter (0.0F, -0.75F, tmpStr);
      sprintf (tmpStr, "%3d", base->Id().num_);
		ShiAssert (strlen(tmpStr) < 32);
      renderer->TextCenter (0.0F, -0.5F, tmpStr);
   }

	if(SimDriver.playerEntity) {
		Tpoint pos;

		pos.x = SimDriver.playerEntity->XPos();
		pos.y = SimDriver.playerEntity->YPos();
		pos.z = SimDriver.playerEntity->ZPos();

		renderer->SetFOV(prevFOV);
		renderer->SetViewport(prevLeft, prevTop, prevRight, prevBottom);
		renderer->SetCamera((struct Tpoint *) &pos, (struct Trotation *) &cameraRot);
	}
}

//**//