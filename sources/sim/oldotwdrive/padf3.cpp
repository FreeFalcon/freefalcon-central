#include "Graphics\Include\renderow.h"
#include "stdhdr.h"
#include "otwdrive.h"
#include "airframe.h"
#include "aircrft.h"
#include "object.h"
#include "simdrive.h"
#include "Graphics\Include\tod.h"
#include "playerop.h"

extern bool g_bNewPadlock;

void OTWDriverClass::PadlockF3_Draw(void)
{
	char tmpStr [_MAX_PATH];


	// Instrument Window
	renderer->SetViewport(padlockWindow[1][0], padlockWindow[1][1], padlockWindow[1][2], padlockWindow[1][3]);
	renderer->SetBackground (0xff000000);
	renderer->ClearFrame();
	renderer->SetColor (0xff00ff00);

	if (otwPlatform) {

		sprintf (tmpStr, "Pct Brk %03.0f : Pct Rpm %03.0f",
			((AircraftClass*)otwPlatform)->af->dbrake*100.0F,
			((AircraftClass*)otwPlatform)->af->rpm*100.0F);
		renderer->TextCenter (0.1F, -0.85F, tmpStr);
	}

	// Diagram Window
	renderer->SetViewport(padlockWindow[2][0], padlockWindow[2][1], padlockWindow[2][2], padlockWindow[2][3]);
	renderer->SetBackground (pVColors[TheTimeOfDay.GetNVGmode() != 0][0]);
	renderer->ClearFrame();

	PadlockF3_DrawSidebar(eyePan, eyeTilt, eyeHeadRoll, renderer);

}

void OTWDriverClass::PadlockF3_MapAnglesToSidebar(float rho, float cmax, float pan, float tilt, float* px, float* py)
{
	mlTrig tiltTrig;
	mlTrig panTrig;

	mlSinCos (&tiltTrig, tilt);
	mlSinCos (&panTrig, pan);

	if(tilt <= 0.0F) {
		*px = rho * tiltTrig.cos * panTrig.sin / cmax;
		*py = -rho * tiltTrig.cos * panTrig.cos / cmax;
	}
	else {
		*px = rho * (2.0F - tiltTrig.cos) * panTrig.sin / cmax;
		*py = -rho * (2.0F - tiltTrig.cos) * panTrig.cos / cmax;
	}
}

void OTWDriverClass::PadlockF3_DrawSidebar(float pan, float tilt, float roll, RenderOTW* pRenderer)
{

	float spacing = 0.04F;
	float spacing1 = 0.01F;
	float y1, y2;
	float x, y;
	float tl_x, tl_y;
	float tr_x, tr_y;
	float bl_x, bl_y;
	float br_x, br_y;
	float aspect;

	mlTrig rotTrig;

	renderer->SetColor (pVColors[TheTimeOfDay.GetNVGmode() != 0][4]);	// Red

	// Zero Line
//   pRenderer->Line (spacing1, mZeroLineRadius, -spacing1, mZeroLineRadius);
   pRenderer->Line (spacing1, -mZeroLineRadius, -spacing1, -mZeroLineRadius);
   pRenderer->Line (mZeroLineRadius, spacing1, mZeroLineRadius, -spacing1);
   pRenderer->Line (-mZeroLineRadius, spacing1, -mZeroLineRadius, -spacing1);

   renderer->SetColor (pVColors[TheTimeOfDay.GetNVGmode() != 0][1]); // Green

	// 30 deg line
//  pRenderer->Line (spacing1, m30LineRadius, -spacing1, m30LineRadius);
   pRenderer->Line (spacing1, -m30LineRadius, -spacing1, -m30LineRadius);
   pRenderer->Line (m30LineRadius, spacing1, m30LineRadius, -spacing1);
   pRenderer->Line (-m30LineRadius, spacing1, -m30LineRadius, -spacing1);

	// 60 deg line
	pRenderer->Line (spacing1, m60LineRadius, -spacing1, m60LineRadius);
   pRenderer->Line (spacing1, -m60LineRadius, -spacing1, -m60LineRadius);
   pRenderer->Line (m60LineRadius, spacing1, m60LineRadius, -spacing1);
   pRenderer->Line (-m60LineRadius, spacing1, -m60LineRadius, -spacing1);

	// Max Line
//  pRenderer->Line (spacing1, mMaxTiltLineRadius, -spacing1, mMaxTiltLineRadius);
   pRenderer->Line (spacing1, -mMaxTiltLineRadius, -spacing1, -mMaxTiltLineRadius);
   pRenderer->Line (mMaxTiltLineRadius, spacing1, mMaxTiltLineRadius, -spacing1);
   pRenderer->Line (-mMaxTiltLineRadius, spacing1, -mMaxTiltLineRadius, -spacing1);

	// Wedge
	pRenderer->Line (mWedgeTipX, mWedgeTipY, mWedgeLeftX, mWedgeY);
	pRenderer->Line (mWedgeTipX, mWedgeTipY, -mWedgeLeftX, mWedgeY);

	// Center Line
	pRenderer->Line (0.0F, mWedgeTipY, 0.0F, -m30LineRadius);

	// single arrow
	y1 = -m45LineRadius;
	y2 = y1 + spacing;
   pRenderer->Line (0.0F, y1, spacing, y2);
   pRenderer->Line (0.0F, y1, -spacing, y2);

	// double arrow
	y1 = -spacing;0.0F;
	y2 = 0.0F;
   pRenderer->Line (0.0F, y1, spacing, y2);
   pRenderer->Line (0.0F, y1, -spacing, y2);

	y1 = y2;
	y2 = y1 + spacing;
   pRenderer->Line (0.0F, y1, spacing, y2);
   pRenderer->Line (0.0F, y1, -spacing, y2);

	// triple arrow
	y1 = m45LineRadius - 1.5F * spacing;
	y2 = y1 + spacing;
   pRenderer->Line (0.0F, y1, spacing, y2);
   pRenderer->Line (0.0F, y1, -spacing, y2);

	y1 = y2;
	y2 = y1 + spacing;
   pRenderer->Line (0.0F, y1, spacing, y2);
   pRenderer->Line (0.0F, y1, -spacing, y2);

	y1 = y2;
	y2 = y1 + spacing;
   pRenderer->Line (0.0F, y1, spacing, y2);
   pRenderer->Line (0.0F, y1, -spacing, y2);

	// Draw tracked target
	renderer->SetColor (pVColors[TheTimeOfDay.GetNVGmode() != 0][2]); // yellow

	// Center
	PadlockF3_MapAnglesToSidebar(mPadRho, mMaxPadC, pan, tilt, &x, &y);

	if(pan > 180.0F * DTR) {
		pan -= 360.0F * DTR;
	}
	else if(pan < -180.0F * DTR) {
		pan += 360.0F * DTR;
	}

	aspect = pRenderer->scaleX / pRenderer->scaleY;

	mlSinCos (&rotTrig, -(pan + roll));
	

	// Center, rotated crosshair
	tl_x = x + (spacing * rotTrig.sin);
	tl_y = y + aspect * (spacing * rotTrig.cos);

	tr_x = x + (-spacing * rotTrig.sin);
	tr_y = y + aspect * (-spacing * rotTrig.cos);

   pRenderer->Line (tl_x, tl_y, tr_x, tr_y);

	tl_x = x + (spacing * rotTrig.cos);
	tl_y = y + aspect * (-(spacing) * rotTrig.sin);

	tr_x = x + (-spacing * rotTrig.cos);
	tr_y = y + aspect * (-(-spacing) * rotTrig.sin);

   pRenderer->Line (tl_x, tl_y, tr_x, tr_y);


	// Rotated Box
	tl_x = x + ((-0.6F) * rotTrig.cos + (0.4F) * rotTrig.sin);
	tl_y = y + aspect * (((0.4F) * rotTrig.cos - (-0.6F) * rotTrig.sin));

	tr_x = x + ((0.6F) * rotTrig.cos + (0.4F) * rotTrig.sin);
	tr_y = y + aspect * (((0.4F) * rotTrig.cos - (0.6F) * rotTrig.sin));

	bl_x = x + ((-0.6F) * rotTrig.cos + (-0.4F) * rotTrig.sin);
	bl_y = y + aspect * (((-0.4F) * rotTrig.cos - (-0.6F) * rotTrig.sin));

	br_x = x + ((0.6F) * rotTrig.cos + (-0.4F) * rotTrig.sin);
	br_y = y + aspect * (((-0.4F) * rotTrig.cos - (0.6F) * rotTrig.sin));
	
   pRenderer->Line (tr_x, tr_y, br_x, br_y); // Right Line
   pRenderer->Line (br_x, br_y, bl_x, bl_y);	// Bottom Line
   pRenderer->Line (bl_x, bl_y, tl_x, tl_y);	// Left Line

   
	renderer->SetColor (pVColors[TheTimeOfDay.GetNVGmode() != 0][3]); // White

   pRenderer->Line (tl_x, tl_y, tr_x, tr_y); // Top Line

}

void OTWDriverClass::PadlockF3_InitSidebar(void)
{

	mlTrig tiltTrig;

	mPadRho		= 1.0F;
	mMinPadTilt = -150.0F * DTR;
	mMaxPadTilt = 25.0F * DTR;
	mMaxPadPan  = 140.0F * DTR;
	mMinPadPan	= -140.0F * DTR;

	mlSinCos(&tiltTrig, mMaxPadTilt);

	mMaxPadC		= mPadRho * (2.0F - tiltTrig.cos);

	PadlockF3_MapAnglesToSidebar(mPadRho, mMaxPadC, 0.0F, -30.0F * DTR, &mWedgeTipY, &m30LineRadius);	//mWedgeTipY is used a dummy variable here
	m30LineRadius			= -m30LineRadius;

	PadlockF3_MapAnglesToSidebar(mPadRho, mMaxPadC, 0.0F, -60.0F * DTR, &mWedgeTipY, &m60LineRadius);	//mWedgeTipY is used a dummy variable here
	m60LineRadius			= -m60LineRadius;

	mZeroLineRadius		= mPadRho / mMaxPadC;
	m45LineRadius			= (float)fabs((mPadRho * 1.41421F) / (2 * mMaxPadC));
	mMaxTiltLineRadius	= 1.0F;

	PadlockF3_MapAnglesToSidebar(mPadRho, mMaxPadC, 0.0F, mMinPadTilt, &mWedgeTipX, &mWedgeTipY);
	PadlockF3_MapAnglesToSidebar(mPadRho, mMaxPadC, mMinPadPan, mMaxPadTilt, &mWedgeLeftX, &mWedgeY);

	PadlockF3_MapAnglesToSidebar(mPadRho, mMaxPadC, 0.0F, mMinPadTilt, &mWedgeTipX, &mWedgeTipY);

//	MapAnglesToDiagram(mPadRho, mMaxPadC, 55.547 * DTR, 0.0, &mWedgeTipX, &mWedgeTipY);

	// This stuff inits values for the blindspot cone behind the pilot's seat
	hBlindArc = 80.0F * DTR;
	vBlindArc = 60.0F * DTR;

	blindA = (float)sin(hBlindArc / 2);
	blindB = (float)sin(vBlindArc / 2);
}



void OTWDriverClass::PadlockF3_CalcCamera(float dT)
{

	mlTrig tiltTrig;
	mlTrig panTrig;
	float tiltSin;
	float tiltLimit;
	float term;

//	if(snapStatus == POSTSNAP && mPadlockTimeout > 0.0F) {
//		return;
//	}

// 2000-11-13 MODIFIED BY S.G. NEED TO CHECK THE RETURN VALUE. IF TRUE, CALC NEW HEAD POSITION. IF FALSE, NOTHING CHANGED SO DON'T MESS AROUND WITH MY HEAD :-) REMAINING OF FUNCTION INCLUDED IN IF BODY.
//	PadlockF3_SetCamera(dT);
	if (PadlockF3_SetCamera(dT)) {

		if(eyePan < mMinPadPan || eyePan > mMaxPadPan) {
		
			term = -(float)sin(180.0F * DTR - fabs(eyePan));
			tiltLimit = -(float)asin(sqrt(blindB * blindB - (term * term * blindB * blindB) / (blindA * blindA)));

			if(eyeTilt > tiltLimit) {
				eyeTilt = tiltLimit;
			}
		}

		eyeTilt	= min(max(eyeTilt, -90.0F * DTR), 25.0F * DTR);

		eyeHeadRoll = 0.0F;
		if(eyePan > 90.0F * DTR || eyePan < -90.0F * DTR) {

			mlSinCos (&panTrig, eyePan);

			if(eyeTilt > 0.0F) {
				tiltSin = 0.0F;
			}
			else {
				mlSinCos (&tiltTrig, -eyeTilt);
				tiltSin = tiltTrig.sin;
			}

			eyeHeadRoll = eyePan * panTrig.cos - (eyePan + eyePan * panTrig.cos) * tiltSin;
		}
		else if (eyeTilt < 0.0F) {
			eyeHeadRoll = -eyePan * (float)sin(-eyeTilt);
		}

		if (g_bNewPadlock)
			BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, 0.0F/*eyeHeadRoll*/);
		else
			BuildHeadMatrix(FALSE, YAW_PITCH, eyePan, eyeTilt, eyeHeadRoll);

		// Combine the head and airplane matrices
		MatrixMult (&ownshipRot, &headMatrix, &cameraRot);
	}
}



//**//

// ----------------------------------------------------------------------------------------------------
//
//	OTWDriverClass::PadlockF3_SlamCamera()
// 
// Arguements:
//		desPan		The desired camera pan angle									(+/- pi)
//		desTilt		The desired camera tilt angle									(+/- pi/2)
//		lagFactor	Rate at which the camera will slew							(0.0 < lagFactor < 1.0)
//		stopCritera	Percentage error at which the camera will stop slewing (0.0 < stop criteria < 1.0)
//		dT				Delta Time in seconds
//
//	Returns:
//		done					done == PAN_AND_TILT;	if both the pan and tilt angles meet the stopCritera
//								done == PAN_ONLY;			if only the pan angle meets the stopCritera
//								done == TILT_ONLY;		if only the tilt angle meets the stopCritera
//								done == NO_PAN_OR_TILT;	if neither pan or tilt meet the stop Critera
//
// This function slew the padlock camera from moving object to moving object.  It tries to maintain
// a constant slew rate by comparing the current error with the previous error.  If the current error is
// greater than the previous, we know that the target object has probably moved away and we need to push
// harder toward the target. If the current error is smaller, we know that the target may have moved toward
// us or that we are about to overshoot it.
//
// ----------------------------------------------------------------------------------------------------
int OTWDriverClass::PadlockF3_SlamCamera(float* prevPRate, float desPan, float* prevPError, float* prevTRate, float desTilt, float* prevTError, float momentum, float stopCritera, float dT)
{

	BOOL panDone	= FALSE;
	BOOL tiltDone	= FALSE;

	float curPError;
	float curTError;
	float currentRate;

	if(*prevPError > stopCritera && stopCritera != 0.0F) {
		curPError	= desPan - eyePan;
		currentRate = (curPError / (*prevPError)) * momentum * *prevPRate;

		if(desPan > eyePan) {
			currentRate = (float)fabs(currentRate);
			eyePan			+= currentRate * dT;
			*prevPRate	= currentRate;
			*prevPError	= curPError;
		}
		else if(desPan < eyePan) {
			currentRate = -(float)fabs(currentRate);
			eyePan			+= currentRate * dT;
			*prevPRate	= currentRate;
			*prevPError	= curPError;
		}
		else {
			panDone	= TRUE;
			eyePan		= desPan;
		}
	}
	else {
		panDone = TRUE;
	}
	

	if(*prevTError > stopCritera && stopCritera != 0.0F) {
		curTError	= desTilt - eyeTilt;
		currentRate = (curTError / (*prevTError)) * momentum * *prevTRate;

		if(desTilt > eyeTilt) {
			currentRate = (float)fabs(currentRate);
			eyeTilt		+= currentRate * dT;
			*prevTRate	= currentRate;
			*prevTError	= curTError;
		}
		else if(desTilt < eyeTilt) {
			currentRate = -(float)fabs(currentRate);
			eyeTilt		+= currentRate * dT;
			*prevTRate	= currentRate;
			*prevTError	= curTError;
		}
		else {
			tiltDone	= TRUE;
			eyeTilt	= desTilt;
		}
	}
	else {
		tiltDone = TRUE;
	}


	return (panDone && tiltDone);
}


//**//

// ----------------------------------------------------------------------------------------------------
//
//	OTWDriverClass::PadlockF3_SlewCamera()
// 
// Arguements:
//		desPan		The desired camera pan angle									(+/- pi)
//		desTilt		The desired camera tilt angle									(+/- pi/2)
//		lagFactor	Rate at which the camera will slew							(0.0 < lagFactor < 1.0)
//		stopCritera	Percentage error at which the camera will stop slewing (0.0 < stop criteria < 1.0)
//		dT				Delta Time in seconds
//
//	Returns:
//		done					done == PAN_AND_TILT;	if both the pan and tilt angles meet the stopCritera
//								done == PAN_ONLY;			if only the pan angle meets the stopCritera
//								done == TILT_ONLY;		if only the tilt angle meets the stopCritera
//								done == NO_PAN_OR_TILT;	if neither pan or tilt meet the stop Critera
//
// This nice function moves the padlock camera to from the current pan and tilt angles to the desired
// the desired pan and tilt angles.  This occurs by gradually slewing the camera in continuous
// fashion instead of popping the camera.  The function relies upon the percentage error of the covered angle,
// i.e. covered angle / desired angle.
//
// ----------------------------------------------------------------------------------------------------

int OTWDriverClass::PadlockF3_SlewCamera(float startPan, float startTilt, float desPan, float desTilt, float lagFactor, float stopCritera, float dT)
{
	BOOL				done			= FALSE;
	static float	oldDesPan	= 0.0F;
	static float	oldDesTilt	= 0.0F;
	float				panErr;
	float				tiltErr;
	float				percentPanErr;
	float				percentTiltErr;
	BOOL				condA;
	BOOL				condB;
	BOOL				condC;
	BOOL				condD;

	// If we are tilted up more than 90 degrees
	if(eyeTilt < -90.0F * DTR) {

		// Calculate the equavalent pan, tilt combination
		eyeTilt = -(180.0F * DTR + eyeTilt);

		if(eyePan >= 0.0F) {
			eyePan -= 180.0F * DTR;
		}
		else {
			eyePan += 180.0F * DTR;
		}
	}

	// Calculate the errors between where we are and where we want to be.
	// Percentage error is how far we have yet to travel divided by total distance.
	panErr			= desPan - eyePan;
	percentPanErr	= (desPan - eyePan) / (startPan - desPan);
	tiltErr			= desTilt - eyeTilt;
	percentTiltErr	= (desTilt - eyeTilt) / (startTilt - desTilt);

	// Handle the discontinuity for the pan at +/- pi
	if(panErr > 180.0F * DTR) {
		panErr -= 360.0F * DTR;
	}
	else if(panErr < -180.0F * DTR) {
		panErr += 360.0F * DTR;
	}

	condA = (startPan == desPan);
	condB = (fabs(percentPanErr) < stopCritera);
	condC = (startTilt == desTilt);
	condD = (fabs(percentTiltErr) < stopCritera);

	// If both pan and tilt are less than the stopCritera
	if((condB && condD) || (condB && condC) || (condA && condD) || (condA && condC)) {
		eyePan	= desPan;		// Close enough, Force the new pan and tilt to be the desired pan and tilt
		eyeTilt	= desTilt;		
		done		= PAN_AND_TILT;
	} // If just the pan is at the stop critera, then make note of it
	else if(condA || condB) {
		done		= PAN_ONLY;
	} // If just the tilt is at the stop critera, then make note of it
	else if(condC || condD) {
		done		= TILT_ONLY;
	}
	else { // If still not there yet
		done		= NO_PAN_OR_TILT;
	}

	// If still slewing
	if(done != PAN_AND_TILT) {

		// Set the new pan angle to be the old pan angle plus some offset.
		// The offset is determined by the size of the error, the elapsed time and
		// an arbitrary rate.
		eyePan = eyePan + (panErr * lagFactor * dT);

		// Handle the discontinuity at +/- pi
		if(eyePan > 180.0F * DTR) {
			eyePan -= 360.0F * DTR;
		}
		else if(eyePan < -180.0F * DTR) {
			eyePan += 360.0F * DTR;
		}

		// Set the new tilt angle to be the old tilt angle plus some offset.
		// The offset is determined by the size of the error, the elapsed time and
		// an arbitrary rate.
		eyeTilt = eyeTilt + (tiltErr * lagFactor * dT);
	}

	// Return our status 
	return done;
}


// 2000-11-13 MODIFIED BY S.G. IF NOTHING CHANGED, SAY SO SO CALLING FUNCTION DOESN'T DO ITS STUFF WHICH MESSES THE HEAD POSITION
//void OTWDriverClass::PadlockF3_SetCamera(float dT) {
int OTWDriverClass::PadlockF3_SetCamera(float dT) {

	SimObjectType* visObj = NULL;
	BOOL haveObj = FALSE;
	float az=0.0F, el=0.0F;

	if(padlockGlance == GlanceNose) {					// if player glances forward

		if(!mIsSlewInit) {
			mIsSlewInit = TRUE;
			mSlewPStart				= eyePan;
			mSlewTStart				= eyeTilt;
		}
		snapStatus = PRESNAP;
		PadlockF3_SlewCamera(mSlewPStart, mSlewTStart, 0.0F, 0.0F, 5.0F, 0.001F, dT);
	}
	else if (padlockGlance == GlanceTail) {			// if player glances back

		snapStatus = PRESNAP;

		if(eyePan < 0.0F) {

			if(!mIsSlewInit) {
				mIsSlewInit = TRUE;
				mSlewPStart				= eyePan;
				mSlewTStart				= eyeTilt;
			}

			PadlockF3_SlewCamera(mSlewPStart, mSlewTStart, -180.0F * DTR,  0.0F, 5.0F, 0.001F, dT);
		}
		else if(eyePan > 0.0F) {

			if(!mIsSlewInit) {
				mIsSlewInit = TRUE;
				mSlewPStart				= eyePan;
				mSlewTStart				= eyeTilt;
			}

			PadlockF3_SlewCamera(mSlewPStart, mSlewTStart, 180.0F * DTR, 0.0F, 5.0F, 0.001F, dT);
		}
		else {
			eyePan	= 0.001F;
		}
	}
	else {

		if(mpPadlockPriorityObject && (snapStatus == PRESNAP || snapStatus == TRACKING)) {
			visObj = ((SimMoverClass*)otwPlatform)->targetList;
		}

		// Find the padlock Obj
		while (visObj && visObj->BaseData() != mpPadlockPriorityObject) {
			visObj = visObj->next;
		}

		if(visObj && visObj->BaseData() == mpPadlockPriorityObject) {
			// find relative location
			CalcRelAzEl (SimDriver.playerEntity, mpPadlockPriorityObject->XPos(), mpPadlockPriorityObject->YPos(), mpPadlockPriorityObject->ZPos(), &az, &el);
			el			= -el;
			haveObj	= TRUE;
		}
		else if(mpPadlockPriorityObject) {
			CalcRelAzEl (SimDriver.playerEntity, mpPadlockPriorityObject->XPos(), mpPadlockPriorityObject->YPos(), mpPadlockPriorityObject->ZPos(), &az, &el);
			el			= -el;
			haveObj	= TRUE;
		}	

		if(haveObj) {

			mObjectOccluded = Padlock_CheckOcclusion(az, el);

			// perform appropiate action
			switch (snapStatus) {
				case PRESNAP:
					int result;
//VWF 2/15/99
/*					if(mObjectOccluded) {
						PadlockOccludedTime += dT;
						if(PadlockOccludedTime >= 5.0F) {
							PadlockOccludedTime = 0.0F;
							VuDeReferenceEntity(mpPadlockPriorityObject);
							mpPadlockPriorityObject = NULL;
							snapStatus = SNAPPING;
						}
						else {
						result = PadlockF3_SlamCamera(&mPrevPRate, az, &mPrevPError, &mPrevTRate, el, &mPrevTError, 1.1F, 0.1F, dT);
						}
					}
					else {
*/
						PadlockOccludedTime = 0.0F;
						result = PadlockF3_SlamCamera(&mPrevPRate, az, &mPrevPError, &mPrevTRate, el, &mPrevTError, 1.1F, 0.1F, dT);

						if(result == PAN_AND_TILT || result == PAN_ONLY) {
							snapStatus = TRACKING;
							//MonoPrint("Switch to TRACKING!\n");
						}
//					}
				break;

				case TRACKING:
					eyePan = az;
					eyeTilt = el;

					if(mObjectOccluded) {
						PadlockOccludedTime += dT;
//	well, you're able to keep a "virtual" padlock for some seconds I'd say...
//						float timer = 5.0F;
//						if (PlayerOptions.GetAvionicsType() == ATRealisticAV)
//							timer = 0.0F;	// in realistic mode, we will instantly loose lock on the target if view is occluded
//						if(PadlockOccludedTime >= timer) {
						if(PadlockOccludedTime >= 5.0F) {
							PadlockOccludedTime = 0.0F;
/* 2001-01-29 MODIFIED BY S.G. FOR THE NEW mpPadlockPrioritySimObject
							VuDeReferenceEntity(mpPadlockPriorityObject);
							mpPadlockPriorityObject = NULL;
*/							SetmpPadlockPriorityObject(NULL);
							snapStatus = SNAPPING;
						}
					}
					else {
						PadlockOccludedTime = 0.0F;
					}
				break;

				case SNAPPING:
// 2000-11-06 REMOVED BY S.G. NO YOU DON'T THIS CODE MAKES THE PADLOCK MOVE TO THE 12h POSITION SOMETIMES WHEN THEIR IS A VALID PADLOCKED OBJECT!
// WE WILL FALL TRHOUGH TO THE NEXT STEP, POSTSNAP WHICH WILL FORCE A PRESNAP (WHICH WILL MAKE IT SNAP IN PLACE)
/*
					if(!mIsSlewInit) {
						mIsSlewInit = TRUE;
						mSlewPStart	= eyePan;
						mSlewTStart	= eyeTilt;
					}


					if(PadlockF3_SlewCamera(mSlewPStart, mSlewTStart, 0.0F, 0.0F, 2.0F, 0.01F, dT) == PAN_AND_TILT) {
						snapStatus = POSTSNAP;
					}
				break;
*/

				case POSTSNAP:
				default:
					mIsSlewInit = FALSE;
					snapStatus = PRESNAP;
					//eyePan		= 0.0F;	
					//eyeTilt		= 0.0F;
				break;
			}
		}
		else {
// 2000-11-06 MODIFIED BY S.G. SO IT DOESN'T SLEW BACK TO CENTER VIEW BUT STAYS AT THE SAME POSITION IF NOTHING IS PADLOCKED
/*			if(snapStatus == PRESNAP || snapStatus == TRACKING) {	// we lost visible object
				snapStatus = SNAPPING;
			}

			if(snapStatus == SNAPPING) {

				if(!mIsSlewInit) {
					mIsSlewInit = TRUE;
					mSlewPStart	= eyePan;
					mSlewTStart	= eyeTilt;
				}

				if(PadlockF3_SlewCamera(mSlewPStart, mSlewTStart, 0.0F, 0.0F, 2.0F, 0.01F, dT) == PAN_AND_TILT) {
					snapStatus	= POSTSNAP;
					mIsSlewInit = FALSE;
				}
			}


			if(snapStatus == POSTSNAP) {

				mIsSlewInit = FALSE;
				eyePan	= 0.0F;
				eyeTilt	= 0.0F;

				SetOTWDisplayMode(OTWDriverClass::Mode3DCockpit);
			}
*/
			// Since SetOTWDisplayMode will reset eyePan, eyeTilt and eyeHeadRoll to zero, I need to keep a copy so I can restore them after the switch to plane 3D
			float tmpEyePan  = eyePan;
			float tmpEyeTilt = eyeTilt;
			float tmpEyeHeadRoll = eyeHeadRoll;
			mIsSlewInit = FALSE;
			SetOTWDisplayMode(OTWDriverClass::Mode3DCockpit);
			eyePan  = tmpEyePan;
			eyeTilt = tmpEyeTilt;
			eyeHeadRoll = tmpEyeHeadRoll;

			return FALSE;
		}
	}

	// ADDED BY S.G. SO THE FUNCTION RETURNS TRUE WHEN IT NEEDS TO RECALCULATE THE HEAD'S POSITION
	return TRUE;
}


