#include "stdafx.h"

#include "cplift.h"
#include "cpmanager.h"
#include "dispopts.h"
#include "playerop.h"
#include "Graphics/Include/renderow.h"
#include "otwdrive.h"

#if CPLIFT_USE_STRING
#include <sstream>

using namespace std;
#endif


CPLiftLine::CPLiftLine(ObjectInitStr* pobjectInitStr, LiftInitStr* liftInitStr) : CPObject(pobjectInitStr)
{
	int i;
	mlTrig trig;
	float panSin, panCos;
	float center;
	float offset;
	float spacing;
	float x1, x2;
	float y1, y2, y3, y4;

	pan	= liftInitStr->pan;
	tilt	= liftInitStr->tilt;
	mDoLabel = liftInitStr->doLabel;
	ostringstream oss;

	if(mDoLabel) {
#if CPLIFT_USE_STRING
		oss << "PAN: " << static_cast<int>(abs(liftInitStr->panLabel)) << " * ";
		if(pan < 0.0F) {
			oss << "left";
		}
		else if(pan > 0.0F){
			oss << "right";
		}
		mString1 = oss.str();
		oss.clear();
		oss << "TILT: " << static_cast<int>(abs(liftInitStr->tiltLabel)) << " * up";
		mString2 = oss.str();
#else
		if(pan < 0.0F) {
			sprintf(mString1, "PAN:  %d * left", abs(liftInitStr->panLabel));
		}
		else if(pan > 0.0F){
			sprintf(mString1, "PAN:  %d * right", abs(liftInitStr->panLabel));
		}
		else {
			sprintf(mString1, "PAN:  0 *");
		}

		sprintf(mString2, "TILT: %d * up", abs(liftInitStr->tiltLabel));
#endif
	}

	if ( PlayerOptions.SimVisualCueMode == VCLiftLine || PlayerOptions.SimVisualCueMode == VCBoth) {

		if(tilt == -30.0F) {
			mLineSegments	= 2;
			mCheverons		= 1;

			center = 0.33F;
			offset = 0.03F;
			spacing = 0.2F;

			x1 = offset;
			x2 = spacing + offset;
			y1 = center + spacing;

			mLineSegment[0][0] = 0.0F;
			mLineSegment[0][1] = 1.0F;
			mLineSegment[1][0] = 0.0F;
			mLineSegment[1][1] = center + offset;		// 0.33 is cheveron center

			mLineSegment[2][0] = 0.0F;
			mLineSegment[2][1] = center - offset;		// 0.33 is cheveron center
			mLineSegment[3][0] = 0.0F;
			mLineSegment[3][1] = 0.0F;

			mCheveron[0][0] = x1;		
			mCheveron[0][1] = center;
			mCheveron[1][0] = x2; 
			mCheveron[1][1] = y1;

			mCheveron[2][0] = -x1;		
			mCheveron[2][1] = center;
			mCheveron[3][0] = -x2; 
			mCheveron[3][1] = y1;
			
			mCheveron[4][0] = 0;
			mCheveron[4][1] = 0;
			mCheveron[5][0] = 0;
			mCheveron[5][1] = 0;

			mCheveron[6][0] = 0;
			mCheveron[6][1] = 0;
			mCheveron[7][0] = 0;
			mCheveron[7][1] = 0;

		}
		else if(tilt == -60.0F) {

			center = 0.33F;
			offset = 0.03F;
			spacing = 0.2F;

			x1 = offset;
			x2 = spacing + offset;

			y1 = center + offset;
			y2 = center - offset;
			
			y3 = -y2;
			y4 = -y1;

			mLineSegments	= 3;
			mCheverons		= 3;

			mLineSegment[0][0] = 0.0F;
			mLineSegment[0][1] = 1.0F;
			mLineSegment[1][0] = 0.0F;
			mLineSegment[1][1] = y1;

			mLineSegment[2][0] = 0.0F;
			mLineSegment[2][1] = y2;
			mLineSegment[3][0] = 0.0F;
			mLineSegment[3][1] = y3;

			mLineSegment[4][0] = 0.0F;
			mLineSegment[4][1] = y4;
			mLineSegment[5][0] = 0.0F;
			mLineSegment[5][1] = -1.0F;

			/////////

			mCheveron[0][0] = x1;
			mCheveron[0][1] = y1;
			mCheveron[1][0] = x2;
			mCheveron[1][1] = y1 + spacing;
		
			mCheveron[2][0] = -x1;
			mCheveron[2][1] = y1;
			mCheveron[3][0] = -x2;
			mCheveron[3][1] = y1 + spacing;

			//////////		

			mCheveron[4][0] = x1;
			mCheveron[4][1] = y2;
			mCheveron[5][0] = x2;
			mCheveron[5][1] = y2 + spacing;

			mCheveron[6][0] = -x1;
			mCheveron[6][1] = y2;
			mCheveron[7][0] = -x2;
			mCheveron[7][1] = y2 + spacing;
			
			/////////

			mCheveron[8][0] = x1;
			mCheveron[8][1] = -center;
			mCheveron[9][0] = x2;
			mCheveron[9][1] = -center + spacing;
			
			mCheveron[10][0] = -x1;
			mCheveron[10][1] = -center;
			mCheveron[11][0] = -x2;
			mCheveron[11][1] = -center + spacing;
		}
		else if(tilt == -90.0F) {
			center = 0.0F;
			offset = 0.03F;
			spacing = 0.2F;

			x1 = offset;
			x2 = spacing + offset;

			y1 = center + offset;
			y2 = center - offset;

			mLineSegments	= 2;
			mCheverons		= 2;

			mLineSegment[0][0] = 0.0F;
			mLineSegment[0][1] = 2.0F;
			mLineSegment[1][0] = 0.0F;
			mLineSegment[1][1] = y1;

			mLineSegment[2][0] = 0.0F;
			mLineSegment[2][1] = y2;
			mLineSegment[3][0] = 0.0F;
			mLineSegment[3][1] = -2.0F;

			//////////
			mCheveron[0][0] = x1;
			mCheveron[0][1] = y1;
			mCheveron[1][0] = x2;
			mCheveron[1][1] = y1 + spacing;
		
			mCheveron[2][0] = -x1;
			mCheveron[2][1] = y1;
			mCheveron[3][0] = -x2;
			mCheveron[3][1] = y1 + spacing;

			//////////		

			mCheveron[4][0] = x1;
			mCheveron[4][1] = y2;
			mCheveron[5][0] = x2;
			mCheveron[5][1] = y2 + spacing;

			mCheveron[6][0] = -x1;
			mCheveron[6][1] = y2;
			mCheveron[7][0] = -x2;
			mCheveron[7][1] = y2 + spacing;
		}

		if(pan != 0.0F) {

			mlSinCos (&trig, pan * DTR);
			panSin = trig.sin;
			panCos = trig.cos;

			for(i = 0; i <= 3; i++) {
				x1 = mLineSegment[i][0] * panCos + mLineSegment[i][1] * panSin;
				y1 = mLineSegment[i][1] * panCos - mLineSegment[i][0] * panSin;
				mLineSegment[i][0] = x1;
				mLineSegment[i][1] = y1;
			}

			for(i = 0; i <= 7; i++) {
				x1 =	mCheveron[i][0] * panCos + mCheveron[i][1] * panSin;
				y1 = mCheveron[i][1] * panCos - mCheveron[i][0] * panSin;
				mCheveron[i][0] =	x1;
				mCheveron[i][1] = y1;
			}
		}
//		else {
//			F4Assert(FALSE);		// Bad parameter
//		}
	}
}

CPLiftLine::~CPLiftLine()
{
}

void CPLiftLine::DisplayDraw(void)
{
	int i;
	int oldFont = VirtualDisplay::CurFont();

//	OTWDriver.renderer->SetColor(0xFF0096FF);
	OTWDriver.renderer->SetColor(OTWDriver.GetLiftLineColor());	// from 3D pit readin

	if ( PlayerOptions.SimVisualCueMode == VCLiftLine || PlayerOptions.SimVisualCueMode == VCBoth) {
		OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);

		for(i = 0; i < mCheverons * 4; i += 4) {
			OTWDriver.renderer->Line(mCheveron[i][0], mCheveron[i][1], mCheveron[i + 1][0], mCheveron[i + 1][1]);
			OTWDriver.renderer->Line(mCheveron[i + 2][0], mCheveron[i + 2][1], mCheveron[i + 3][0], mCheveron[i + 3][1]);
		}
		for(i = 0; i < mLineSegments * 2; i += 2) {
			OTWDriver.renderer->Line(mLineSegment[i][0], mLineSegment[i][1], mLineSegment[i + 1][0], mLineSegment[i + 1][1]);
		}
	}

	if(mDoLabel) {
		OTWDriver.renderer->SetViewport( 0.75F, 1.0F, 1.0F, 0.87F );
		OTWDriver.renderer->SetBackground(0x00000000);
		OTWDriver.renderer->ClearDraw();

		OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
		OTWDriver.renderer->SetColor(0xFF00FF00);
		VirtualDisplay::SetFont(mpCPManager->SABoxFont());
#if CPLIFT_USE_STRING
		OTWDriver.renderer->TextLeft( 0.78F, 0.97F, mString1.c_str());
		OTWDriver.renderer->TextLeft( 0.78F, 0.92F, mString2.c_str());
#else
		OTWDriver.renderer->TextLeft( 0.78F, 0.97F, mString1);
		OTWDriver.renderer->TextLeft( 0.78F, 0.92F, mString2);
#endif
		VirtualDisplay::SetFont(oldFont);
	}
}