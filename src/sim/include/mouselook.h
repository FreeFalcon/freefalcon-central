/************************************************************************/
//	Filename:	mouselook.h
//	Date:		18Jan2004
//	Author:		Retro
//
//	Description;	See the .cpp
/************************************************************************/
#ifndef MOUSELOOK_INCLUDE_INCLUDED
#define MOUSELOOK_INCLUDE_INCLUDED

#include "SimIO.h"

/************************************************************************/
//	
/************************************************************************/
class MouseView {
public:
	MouseView();

	float GetMouseAzim();
	float GetMouseElev();

	void Reset();

	void AddAzimuth(float);
	void AddElevation(float);

	void BumpViewUp(float);
	void BumpViewLeft(float);

	void Compute(float,bool mouseMoved = false);
private:
	float XTotal;	// total travel values, clamped to a range..
	float YTotal;

	float Azimuth;	// normalized view angels (+-PI in RAD)
	float Elevation;

	float azDir;	// directions.. -1 is right/down, 1 is left/up, 0 stops
	float elDir;
};
extern MouseView theMouseView;

/************************************************************************/
// Retro 18Jan2004 - support for mousewheel as fake axis
/************************************************************************/
class MouseWheelStuff {
public:
	MouseWheelStuff();

	void SetAxis(GameAxis_t);
	void ResetAxisValue();
	long GetAxisValue();
	void AddToAxisValue(long);

	void SetWheelInactive() { WheelIsUsed = false; }
	bool IsWheelActive() { return WheelIsUsed; }

private:
	GameAxis_t theMappedAxis;
	bool isUnipolar;
	long theAxisValue;
	bool WheelIsUsed;
};
extern MouseWheelStuff theMouseWheelAxis;

#endif	// MOUSELOOK_INCLUDE_INCLUDED