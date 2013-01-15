/************************************************************************/
//	Filename:	mouselook.cpp
//	Date:		18Jan2004
//	Author:		Retro
//
//	Description;	2 Classes are defined in this file
//		MouseView		Handles joint EXTERNAL view panning via 
//						mouse and keyboard / POV
//		MouseWheelStuff	Emulates an absolute physical axis from
//						the mousewheel axis
//
//		This file also holds 2 global objects (1 of each class)
/************************************************************************/
#include "SimIO.h"
#include "mouselook.h"

/************************************************************************/
//	just a number. ASSuming mouse granularity of 20 and sensitivity
//	of 5 ( == 0.5 * 10)
/************************************************************************/
static const int MAX_AXIS_THROW = (150*10);

/************************************************************************/
//	MouseView class:
//	Handles joint view panning via mouse and keyboard / POV
/************************************************************************/
MouseView::MouseView()
{
	XTotal = 0;
	YTotal = 0;
	Azimuth = 0;
	Elevation = 0;

	azDir = 0.;
	elDir = 0.;
}

/************************************************************************/
//	Resets all view values
/************************************************************************/
void MouseView::Reset()
{
	XTotal = 0;
	YTotal = 0;
	Azimuth = 0;
	Elevation = 0;

	azDir = 0.;
	elDir = 0.;
}

/************************************************************************/
//	Gets the values for pan and pitch
/************************************************************************/
float MouseView::GetMouseAzim() { return Azimuth; }
float MouseView::GetMouseElev() { return Elevation; }

/************************************************************************/
//	Add mouse azimuth delta values to total sum
/************************************************************************/
void MouseView::AddAzimuth(float theVal)
{
	XTotal += theVal;
}

/************************************************************************/
//	Add mouse elevation delta values to total sum
/************************************************************************/
void MouseView::AddElevation(float theVal)
{
	YTotal += theVal;
}

/************************************************************************/
//	Slew view up/down by keypress / POV. 0 to stop
/************************************************************************/
void MouseView::BumpViewUp(float direction)
{
	elDir = direction;
}

/************************************************************************/
//	Slew view left/right by keypress /POV. 0 to stop
/************************************************************************/
void MouseView::BumpViewLeft(float direction)
{
	azDir = direction;
}

/************************************************************************/
//	Compute the new azimuth/elevation angles from the combined
//	mouse and keyboard / POV inputs. This function is called in 2 places
//	1) in the mouse routine that gets invoked whenever new mouse data
//		is available (bool mouseMoved is TRUE)
//	2) in the external view draw routine. (bool mouseMoved is TRUE)
//		only does something if keypress / POB move is active
/************************************************************************/
void MouseView::Compute(float amount, bool mouseMoved)
{
	if ((azDir)||mouseMoved)
	{
		if (!mouseMoved)
			XTotal += (int)(1500.f * amount * azDir);	// 1500.f is an empiric value..

		if (XTotal > MAX_AXIS_THROW)
			XTotal -= 2*MAX_AXIS_THROW;
		else if (XTotal < -MAX_AXIS_THROW)
			XTotal += 2*MAX_AXIS_THROW;

		Azimuth = XTotal / MAX_AXIS_THROW * PI;
	}

	if ((elDir)||mouseMoved)
	{
		if (!mouseMoved)
			YTotal += (int)(1500.f * amount * elDir);	// 1500.f is an empiric value..

		if (YTotal > MAX_AXIS_THROW)
			YTotal -= 2*MAX_AXIS_THROW;
		else if (YTotal < -MAX_AXIS_THROW)
			YTotal += 2*MAX_AXIS_THROW;

		Elevation = YTotal / MAX_AXIS_THROW * PI;
	}
}
MouseView theMouseView;

/************************************************************************/
//	Class MouseWheelStuff
//	Emulates an absolute physical axis from the mousewheel axis
//	As I can´t set the range of the axis via DX (like I do with 'real' axis)
//	I have to do this manually here.
//	Also, the axis can be either unipolar or bipolar
/************************************************************************/
MouseWheelStuff::MouseWheelStuff()
{
	isUnipolar = false;
	theAxisValue = 0;
	theMappedAxis = AXIS_START;

	WheelIsUsed = false;
}

extern GameAxisSetup_t AxisSetup[AXIS_MAX];
/************************************************************************/
//	Note the axis the mousewheel is currently mapped to. This has
//	consequences for the value range that gets returned.
/************************************************************************/
void MouseWheelStuff::SetAxis(GameAxis_t theAxis)
{
	theMappedAxis = theAxis;
	isUnipolar = AxisSetup[theAxis].isUniPolar;
	WheelIsUsed = true;
}

/************************************************************************/
//	Update 'virtual' axis values with new mouse data
/************************************************************************/
void MouseWheelStuff::AddToAxisValue(long theVal)
{
	theAxisValue += theVal;
	if (isUnipolar)
		theAxisValue = max ( min (theAxisValue, 15000), 0);
	else
		theAxisValue = max ( min (theAxisValue, 10000), -10000);
}

/************************************************************************/
//	returns the value of the axis
/************************************************************************/
long MouseWheelStuff::GetAxisValue()
{
	return theAxisValue;
}

extern float g_fDefaultFOV;	// Wombat778 10-31-2003
extern float g_fMaximumFOV;	// Wombat778 1-15-03
extern float g_fMinimumFOV;	// Wombat778 1-15-03

/************************************************************************/
//	reset the value of the axis
//	) for 'normal' unipolar axis it´s just one axis extreme
//	) for 'normal' bipolar axis it´s zero
//	) for some special axis the value gets computed here. These are axis
//		where default axis values are given (eg FOV min or max would be
//		quite hard on the users so we reset to a middle-ranged default value)
/************************************************************************/
void MouseWheelStuff::ResetAxisValue()
{
	switch (theMappedAxis)
	{
	case AXIS_FOV:
		{
			// should be default FOV scaled to 0-15000 !!
			theAxisValue = (long)(((float)(g_fDefaultFOV) / (float)g_fMaximumFOV) * 15000.f);
			break;
		}
	case AXIS_ZOOM:
		{
			theAxisValue = (long)(15000.f/900.f * 75.f);	// 75 feet(?) default zoom range
			break;
		}
	default:
		if (isUnipolar)
			theAxisValue = 7500;	// just go to the middle of the range..
		else
			theAxisValue = 0;		// just pick one extreme (user can reverse axis anyway)
		break;
	}
}
MouseWheelStuff theMouseWheelAxis;
