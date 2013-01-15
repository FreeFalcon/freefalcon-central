/***************************************************************************\
    Scripts.cpp
    Scott Randolph
    April 4, 1998

    Provides custom code for use by specific BSPlib objects.
\***************************************************************************/
#include "stdafx.h"
#include <math.h>
#include "TimeMgr.h"
#include "StateStack.h"
#include "ObjectInstance.h"
#include "Scripts.h"
#include "falclib\include\mlTrig.h"

// Some handy constants to get things into units of ms and Radians
static const float	Seconds				= 1.0f / 1000.0f;
static const float	Minutes				= Seconds / (60.0f);
static const float	Hours				= Minutes / (60.0f);
static const float	Degrees				= (PI/180.0f);
static const float	DegreesPerSecond	= Degrees * Seconds;



/********************************************\
	These are the bodies of the custom
	objects scripts.
	MAKE SURE TO ADD REFERENCES to any new
	scripts to the list at the end of this
	file and to ScriptNames.CPP.
\********************************************/

// Apache main rotor and tail rotor rotation
static void AH64(void)
{
	float delta = TheTimeManager.GetDeltaTime() * 540.0f * DegreesPerSecond;

	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nDOFs > 4 );
	if (TheStateStack.CurrentInstance->ParentObject->nDOFs <= 4) return;

	TheStateStack.CurrentInstance->DOFValues[2].rotation += delta;		// Main rotor
	TheStateStack.CurrentInstance->DOFValues[3].rotation += delta * 1.6f;	// Tail rotor
	TheStateStack.CurrentInstance->DOFValues[4].rotation += delta * 2.1f;	// Muzzle flash
}

// Four propellors spinning
static void C130(void)
{
	float delta = TheTimeManager.GetDeltaTime() * 200.0f * DegreesPerSecond;

	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nDOFs > 5 );
	if (TheStateStack.CurrentInstance->ParentObject->nDOFs <= 5) return;

	TheStateStack.CurrentInstance->DOFValues[2].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[3].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[4].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[5].rotation += delta;
}

// Approach angle apprpriate VASI light indications (FAR set)
static void VASIF(void)
{
	float angle = (float)atan2( -TheStateStack.ObjSpaceEye.z, TheStateStack.ObjSpaceEye.x );

	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nSwitches > 0 );
	if (TheStateStack.CurrentInstance->ParentObject->nSwitches <= 0) return;

	if (angle > 4.0f * Degrees) {
		TheStateStack.CurrentInstance->SetSwitch( 0, 2 );	// White
	} else {
		TheStateStack.CurrentInstance->SetSwitch( 0, 1 );	// Red
	}
}

// Approach angle apprpriate VASI light indications (NEAR set)
static void VASIN(void)
{
	float angle = (float)atan2( -TheStateStack.ObjSpaceEye.z, TheStateStack.ObjSpaceEye.x );

	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nSwitches > 0 );
	if (TheStateStack.CurrentInstance->ParentObject->nSwitches <= 0) return;

	if (angle > 2.0f * Degrees) {
		TheStateStack.CurrentInstance->SetSwitch( 0, 2 );	// White
	} else {
		TheStateStack.CurrentInstance->SetSwitch( 0, 1 );	// Red
	}
}

// Chaff animation
// TODO:  Should run at 10hz, not frame rate (ie be time based not frame based)
static void Chaff(void)
{
	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nSwitches > 0 );
	if (TheStateStack.CurrentInstance->ParentObject->nSwitches <= 0) return;

	if (TheStateStack.CurrentInstance->SwitchValues[0] == 0)
	{
		TheStateStack.CurrentInstance->SwitchValues[0] = 1;
	}
//	else if (TheStateStack.CurrentInstance->SwitchValues[0] < 0x80)
//	{
//		TheStateStack.CurrentInstance->SwitchValues[0] <<= 1;
//	}
}

// Military rotating beacon script
// Assume model has green pointing north, and two whites 30 degrees apart pointing southward
// switch bits:
//		1	0   degree green dim
//		2	0   degree green flash
//		3	0   degree green has flashed flag
//		5	165 degree white dim
//		6	165 degree white flash
//		7	165 degree white has flashed flag
//		9	195 degree white dim
//		10	195 degree white flash
//		11	195 degree white has flashed flag
static void Beacon(void)
{
	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nSwitches > 1 );
	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nDOFs > 0 );
	if (TheStateStack.CurrentInstance->ParentObject->nDOFs <= 0) return;
	if (TheStateStack.CurrentInstance->ParentObject->nSwitches <= 1) return;

	DWORD sw = TheStateStack.CurrentInstance->SwitchValues[1];
	float delta = TheTimeManager.GetDeltaTime() * 36.0f * DegreesPerSecond;
	float rot;			// Rotation of beacon head
	float dx, dy;		// Eye vector in light beam space
	float da;			// Signed angle between light beam and vector to the eye point
	mlTrig trig;


//	delta = 0.0f;

	// Compute the beacon's orientation and angle away from the viewer
	rot = TheStateStack.CurrentInstance->DOFValues[0].rotation + delta;

	// 0 degree green light
	mlSinCos(&trig, rot);
	dx =  TheStateStack.ObjSpaceEye.x * trig.cos + TheStateStack.ObjSpaceEye.y * trig.sin;
	dy = -TheStateStack.ObjSpaceEye.x * trig.sin + TheStateStack.ObjSpaceEye.y * trig.cos;
	da = (float)atan2( dy, dx );

	if (fabs(da) > 5.0f*Degrees) {
		sw &= 0xFFFFFFF8;				// All off
	} else {
		if (da < 0.0f) {
			if ((sw & 0x4) == 0) {
				sw |= 0x7;				// Flash on, has flashed, visible
			} else {
				sw &= 0xFFFFFFFD;		// Flash off
			}
		} else {
			sw |= 0x1;					// Visible
		}
	}

	// 165 degree white light
	mlSinCos(&trig, rot + 165.0F*Degrees);
	dx =  TheStateStack.ObjSpaceEye.x * trig.cos + TheStateStack.ObjSpaceEye.y * trig.sin;
	dy = -TheStateStack.ObjSpaceEye.x * trig.sin + TheStateStack.ObjSpaceEye.y * trig.cos;
	da = (float)atan2( dy, dx );

	if (fabs(da) > 5.0f*Degrees) {
		sw &= 0xFFFFFF8F;				// All off
	} else {
		if (da < 0.0f) {
			if ((sw & 0x40) == 0) {
				sw |= 0x70;				// Flash on, has flashed, visible
			} else {
				sw &= 0xFFFFFFDF;		// Flash off
			}
		} else {
			sw |= 0x10;					// Visible
		}
	}

	// 195 degree white light
	mlSinCos(&trig, rot + 195.0F * Degrees);
	dx =  TheStateStack.ObjSpaceEye.x * trig.cos + TheStateStack.ObjSpaceEye.y * trig.sin;
	dy = -TheStateStack.ObjSpaceEye.x * trig.sin + TheStateStack.ObjSpaceEye.y * trig.cos;
	da = (float)atan2( dy, dx );

	if (fabs(da) > 5.0f*Degrees) {
		sw &= 0xFFFFF8FF;				// All off
	} else {
		if (da < 0.0f) {
			if ((sw & 0x400) == 0) {
				sw |= 0x700;			// Flash on, has flashed, visible
			} else {
				sw &= 0xFFFFFDFF;		// Flash off
			}
		} else {
			sw |= 0x100;				// Visible
		}
	}

	// Now store the computed results
	TheStateStack.CurrentInstance->DOFValues[0].rotation = (float)fmod( rot, 2.0f*PI );
	TheStateStack.CurrentInstance->SwitchValues[1] = sw;
}

// Death of ejected pilot sequence
// TODO:  Should be .187 fps, not dependent on frame rate (ie be time based not frame based)
static void CollapseChute(void)
{
	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nSwitches > 0 );

	if (TheStateStack.CurrentInstance->SwitchValues[0] == 0) {
		TheStateStack.CurrentInstance->SwitchValues[0] = 1;
	} else if (TheStateStack.CurrentInstance->SwitchValues[0] < 0x40) {
		TheStateStack.CurrentInstance->SwitchValues[0] <<= 1;
	} else if (TheStateStack.CurrentInstance->SwitchValues[0] == 0x40) {
		TheStateStack.CurrentInstance->SwitchValues[0] = 0x10000020;
	} else if (TheStateStack.CurrentInstance->SwitchValues[0] == 0x10000020) {
		TheStateStack.CurrentInstance->SwitchValues[0] = 0x10000010;
	} else {
		TheStateStack.CurrentInstance->SwitchValues[0] = 0x00000008;
	}
}

// E3 Rotor dome
static void E3(void)
{
	float delta = TheTimeManager.GetDeltaTime() * 135.0f * DegreesPerSecond;

	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nDOFs > 3 );
	if (TheStateStack.CurrentInstance->ParentObject->nDOFs <= 3) return;

	TheStateStack.CurrentInstance->DOFValues[14].rotation += delta;
}

// Heuy main rotor and tail rotor rotation
static void UH1(void)
{
	float delta = TheTimeManager.GetDeltaTime() * 500.0f * DegreesPerSecond;

	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nDOFs > 3 );
	if (TheStateStack.CurrentInstance->ParentObject->nDOFs <= 3) return;

	TheStateStack.CurrentInstance->DOFValues[2].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[3].rotation += delta*2.1f;
}

// Heuy main rotor and tail rotor rotation
static void LongBow(void)
{
	float delta = TheTimeManager.GetDeltaTime() * 540.0f * DegreesPerSecond;

	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nDOFs > 5 );
	if (TheStateStack.CurrentInstance->ParentObject->nDOFs <= 5) return;

	TheStateStack.CurrentInstance->DOFValues[2].rotation += delta;			// Main rotor
	TheStateStack.CurrentInstance->DOFValues[3].rotation += delta * 1.6f;	// Tail rotor
	TheStateStack.CurrentInstance->DOFValues[4].rotation += delta * 2.1f;	// Muzzle flash
	TheStateStack.CurrentInstance->DOFValues[5].rotation += delta * 0.1f;	// Radar
}

// Hokum main rotor rotations
static void Hokum(void)
{
	float delta = TheTimeManager.GetDeltaTime() * 360.0f * DegreesPerSecond;

	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nDOFs > 4 );
	if (TheStateStack.CurrentInstance->ParentObject->nDOFs <= 4) return;

	TheStateStack.CurrentInstance->DOFValues[2].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[3].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[4].rotation += delta;
}

// Single engine prop rotation
static void OneProp(void)
{
	float delta = TheTimeManager.GetDeltaTime() * 600.0f * DegreesPerSecond;

	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nDOFs > 2 );
	if (TheStateStack.CurrentInstance->ParentObject->nDOFs <= 2) return;

	TheStateStack.CurrentInstance->DOFValues[2].rotation += delta;
}

// Two frame animation cycle
// TODO:  Possibly make this time based instead of frame based.
static void Cycle2(void)
{
	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nSwitches > 0 );
	if (TheStateStack.CurrentInstance->ParentObject->nSwitches <= 0) return;

	if (TheStateStack.CurrentInstance->SwitchValues[0] == 1) {
		TheStateStack.CurrentInstance->SwitchValues[0] = 2;
	} else {
		TheStateStack.CurrentInstance->SwitchValues[0] = 1;
	}
}

// Four frame animation cycle
// TODO:  Possibly make this time based instead of frame based.
static void Cycle4(void)
{
	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nSwitches > 0 );
	if (TheStateStack.CurrentInstance->ParentObject->nSwitches <= 0) return;

	if (TheStateStack.CurrentInstance->SwitchValues[0] >= 8) {
		TheStateStack.CurrentInstance->SwitchValues[0] = 1;
	} else if (TheStateStack.CurrentInstance->SwitchValues[0] > 0) {
		TheStateStack.CurrentInstance->SwitchValues[0] <<= 1;
	} else {
		TheStateStack.CurrentInstance->SwitchValues[0] = 1;
	}
}

// Ten frame animation cycle
// TODO:  Possibly make this time based instead of frame based.
static void Cycle10(void)
{
	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nSwitches > 0 );
	if (TheStateStack.CurrentInstance->ParentObject->nSwitches <= 0) return;

	if (TheStateStack.CurrentInstance->SwitchValues[0] >= 0x200) {
		TheStateStack.CurrentInstance->SwitchValues[0] = 1;
	} else if (TheStateStack.CurrentInstance->SwitchValues[0] > 0) {
		TheStateStack.CurrentInstance->SwitchValues[0] <<= 1;
	} else {
		TheStateStack.CurrentInstance->SwitchValues[0] = 1;
	}
}

// Animation for F16 strobe light
// TODO:  Possibly make this time based instead of frame based.
static void TStrobe(void)
{
	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nSwitches > 7 );
	if (TheStateStack.CurrentInstance->ParentObject->nSwitches <= 7) return;
	
	if (TheStateStack.CurrentInstance->SwitchValues[7] >= 0x10) {
		TheStateStack.CurrentInstance->SwitchValues[7] = 1;
	} else if (TheStateStack.CurrentInstance->SwitchValues[7] > 0) {
		TheStateStack.CurrentInstance->SwitchValues[7] <<= 1;
	}
}


static void TU95(void)
{
	float delta = TheTimeManager.GetDeltaTime() * 200.0f * DegreesPerSecond;

	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nDOFs > 9 );
	if (TheStateStack.CurrentInstance->ParentObject->nDOFs <= 9) return;

	TheStateStack.CurrentInstance->DOFValues[2].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[3].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[4].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[5].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[6].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[7].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[8].rotation += delta;
	TheStateStack.CurrentInstance->DOFValues[9].rotation += delta;
}

// Approach angle for Carrier MeatBall - 13 switches (0-12) for vertical Glide Slope
static void MeatBall(void)
{
	float angle = (float)atan2( -TheStateStack.ObjSpaceEye.z, TheStateStack.ObjSpaceEye.x );
	static const float GS = 3;
	static const float angles[] = 
	{
		GS+2.3f, GS+2, GS+1.7f, GS+1.3f, GS+1, GS+0.7F, GS+0.3f, 
		GS, 
		GS-0.3f, GS-0.7f, GS-1, GS-1.3f, GS-1.7f
	};
	static const int NANGLES = sizeof(angles)/sizeof(angles[0]);

	ShiAssert( TheStateStack.CurrentInstance->ParentObject->nSwitches >= NANGLES -2);
	if (TheStateStack.CurrentInstance->ParentObject->nSwitches <= NANGLES -2) return;
	angle /= Degrees;
	for (int i = 0; i < NANGLES-1; i++) 
	{
	  if (angle < angles[i] && angle > angles[i+1])
			TheStateStack.CurrentInstance->SetSwitch(i, 1);
	  else
			TheStateStack.CurrentInstance->SetSwitch(i, 0);
	}
}

static void ComplexProp(void)
{
	float delta = TheTimeManager.GetDeltaTime() * 200.0f * DegreesPerSecond;

	int maxd = TheStateStack.CurrentInstance->ParentObject->nDOFs;
	ShiAssert( maxd > 31);
	maxd = min(maxd, 38);
	for (int i = 31; i < maxd; i++)
		TheStateStack.CurrentInstance->DOFValues[i].rotation += delta;
}
/********************************************\
	These are the two publicly visible
	elements of the custom script utility.

	MAKE SURE TO ADD SCRIPTS HERE 
	AND IN ScriptNames.cpp
\********************************************/
ScriptFunctionPtr ScriptArray[] = {
	UH1,
	AH64,
	Hokum,
	OneProp,
	C130,
	E3,
	VASIF,
	VASIN,
	Chaff,
	Beacon,
	CollapseChute,
	LongBow,
	Cycle2,
	Cycle4,
	Cycle10,
	TStrobe,
	TU95,
	MeatBall,
	ComplexProp,
};

int	ScriptArrayLength = sizeof(ScriptArray) / sizeof(*ScriptArray);
