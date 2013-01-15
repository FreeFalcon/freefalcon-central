#include "stdafx.h"
#include "falclib.h"
#include "dispcfg.h"
#include "cpmachasi.h"
#include "Graphics/Include/renderow.h"
#include "otwdrive.h"


//-----------------------------------------------------------------------------
// CPMachAsi::CPMachAsi
//-----------------------------------------------------------------------------

CPMachAsi::CPMachAsi(ObjectInitStr *pobjectInitStr, MachAsiInitStr* pmachAsiInitStr) : CPObject(pobjectInitStr) {

	mxCenter								= mDestRect.left + (mDestRect.right - mDestRect.left) / 2;
	myCenter								= mDestRect.top + (mDestRect.bottom - mDestRect.top) / 2;
												
	mMinimumDialValue						= pmachAsiInitStr->min_dial_value;
	mMaximumDialValue						= pmachAsiInitStr->max_dial_value;
	mDialStartAngle							= pmachAsiInitStr->dial_start_angle;
	mDialArcLength							= pmachAsiInitStr->dial_arc_length;
	// sfr: use smaller scaling value here
	mNeedleRadius							= (int)(pmachAsiInitStr->needle_radius * (mHScale < mVScale) ? mHScale : mVScale);
	mEndLength								= pmachAsiInitStr->end_radius;
	mEndAngle		                        = pmachAsiInitStr->end_angle;

	mColor[0][0]							= pmachAsiInitStr->color0;
	mColor[1][0]							= CalculateNVGColor(mColor[0][0]);
	mColor[0][1]							= pmachAsiInitStr->color1;
	mColor[1][1]							= CalculateNVGColor(mColor[0][1]);
}



//-----------------------------------------------------------------------------
// CPMachAsi::~CPMachAsi
//-----------------------------------------------------------------------------

CPMachAsi::~CPMachAsi() {
}



//-----------------------------------------------------------------------------
// CPMachAsi::Exec
//-----------------------------------------------------------------------------

void CPMachAsi::Exec(SimBaseClass*) {
	
	if (mExecCallback) {
		mExecCallback(this);	// Get Airspeed and Mach number from the airframe
	}
	CalculateDeflection();
	CalculateNeedlePosition();
}



//-----------------------------------------------------------------------------
// CPMachAsi::CalculateDeflection
//-----------------------------------------------------------------------------

void CPMachAsi::CalculateDeflection(void) {

	// Bound the values to the range of the dial
	mAirSpeed		= min(mAirSpeed, mMaximumDialValue);
	mAirSpeed		= max(mAirSpeed, mMinimumDialValue);

	// ASI Instrument has a Log10 scale.  Deflection is a fraction 
	// of total scale arc length plus the start angle of ASI scale.
	mDeflection		= mDialStartAngle - (float)(log10(mAirSpeed) * mDialArcLength);

	// Bound Angle of deflection to +- PI
	if(mDeflection < -PI) {
		mDeflection	+= (2 * PI);
	}
	else if(mDeflection > PI){
		mDeflection	-= (2 * PI);
	}
}



//-----------------------------------------------------------------------------
// CPMachAsi::CalculateNeedlePosition
//-----------------------------------------------------------------------------

void CPMachAsi::CalculateNeedlePosition(void) {
mlTrig  trig;

	// Needle is drawn as two triangles rotated to the angle of deflection.
	// Calculate the verticies of the triangles.
   mlSinCos (&trig, mDeflection);
	mxNeedlePos1	= mxCenter + FloatToInt32(mNeedleRadius * trig.cos);
	myNeedlePos1	= myCenter - FloatToInt32(mNeedleRadius * trig.sin);

   mlSinCos (&trig, mDeflection - 180.0F * DTR + mEndAngle);
	mxNeedlePos2	= mxCenter + FloatToInt32 (mNeedleRadius * mEndLength * trig.cos);
	myNeedlePos2	= myCenter - FloatToInt32 (mNeedleRadius * mEndLength * trig.sin);

   mlSinCos (&trig, mDeflection - 180.0F * DTR - mEndAngle);
	mxNeedlePos3	= mxCenter + FloatToInt32 (mNeedleRadius * mEndLength * trig.cos);
	myNeedlePos3	= myCenter - FloatToInt32 (mNeedleRadius * mEndLength * trig.sin);
}







//-----------------------------------------------------------------------------
// CPMachAsi::DisplayDraw
//-----------------------------------------------------------------------------

void CPMachAsi::DisplayDraw() {

	mDirtyFlag = TRUE;

	if(!mDirtyFlag) {
		return;
	}
	
	// compute color with light applied
	DWORD color[2]; // derived from original color
	for (int i=0;i<2;i++){
		color[i] = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][i], true);
	}

	// Draw the needle.  Needle is created by drawing two triangles back to back.
	OTWDriver.renderer->SetColor(color[0]);

	OTWDriver.renderer->Render2DTri(
		(float)mxCenter, (float)myCenter, (float)mxNeedlePos1,
		(float)myNeedlePos1, (float)mxNeedlePos2, (float)myNeedlePos2
	);
	OTWDriver.renderer->SetColor(color[1]);

	OTWDriver.renderer->Render2DTri(
		(float)mxCenter, (float)myCenter, (float)mxNeedlePos1,
		(float)myNeedlePos1, (float)mxNeedlePos3, (float)myNeedlePos3
	);

	mDirtyFlag = FALSE;
}













