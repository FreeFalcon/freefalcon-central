#include "falclib.h"
#include "Graphics\Include\grtypes.h"
#include "vdial.h"
#include "cpmanager.h"
#include "Graphics\Include\renderow.h"
#include "Graphics\Include\Canvas3d.h"
#include "Graphics\Include\tod.h"

VDial::VDial(VDialInitStr* pInit) {

	int		i;
   mlTrig   trig;

	mCallback	= pInit->callback;
	mRadius		= pInit->radius;
	mColor		= pInit->color;
	mEndPoints	= pInit->endPoints;
	mpValues = new float[mEndPoints];
	mpPoints = new float[mEndPoints];
	memcpy(mpValues, pInit->pvalues, sizeof(*mpValues) * mEndPoints);
	memcpy(mpPoints, pInit->ppoints, sizeof(*mpPoints) * mEndPoints);

	mNVGColor	= CalculateNVGColor(mColor);

	mDialValue	= 0.0F;

	mpCanvas		= new Canvas3D;
	mpCanvas->Setup(pInit->pRender);
	mpCanvas->SetCanvas(pInit->pUL, pInit->pUR, pInit->pLL);
	mpCanvas->Update( (struct Tpoint *)&Origin, (struct Trotation *)&IMatrix );


	#ifdef USE_SH_POOLS
	mpCosPoints = (float *)MemAllocPtr(gCockMemPool,sizeof(float)*mEndPoints,FALSE);
	mpSinPoints = (float *)MemAllocPtr(gCockMemPool,sizeof(float)*mEndPoints,FALSE);
	#else
	mpCosPoints	= new float[mEndPoints];
	mpSinPoints	= new float[mEndPoints];
	#endif

	for(i = 0; i < mEndPoints; i++) {
      mlSinCos (&trig, mpPoints[i]);

		mpCosPoints[i]	= trig.cos;
		mpSinPoints[i] = trig.sin;
	}

	// JPO - lets be careful out there.
	if(mCallback < 0 || mCallback >= TOTAL_CPCALLBACK_SLOTS) {
		mExecCallback		= NULL;
	}
	else {
		mExecCallback		= CPCallbackArray[mCallback].ExecCallback;
	}
}


VDial::~VDial() {
	delete mpCanvas;

	delete [] mpSinPoints;
	delete [] mpCosPoints;
	delete [] mpValues;
	delete [] mpPoints;
}


void VDial::Exec(SimBaseClass* pOwnship){
	BOOL		found = FALSE;
	int			i= 0;
	float		x=0.0F;
	float		y=0.0F;
	float		delta=0.0F;
	float		slope=0.0F;
	float		deflection=0.0F;
	float		cosDeflection=1.0F;
	float		sinDeflection=0.0F;
	mlTrig		trig;

	mpOwnship = pOwnship;

	//get the value from ac model
	if(mExecCallback) {
		mExecCallback(this);
	}
	else {
		return;
	}

	if (TheTimeOfDay.GetNVGmode() ==0)
	    mpCanvas->SetColor(mColor);
	else 
	    mpCanvas->SetColor(mNVGColor);

	do {
		if(mDialValue <= mpValues[0]) {
			found		= TRUE;
			x			= mRadius * mpCosPoints[0];
			y			= -mRadius * mpSinPoints[0];
  		}
		else if(mDialValue >= mpValues[mEndPoints - 1]) {
			found		= TRUE;
			x			= mRadius * mpCosPoints[mEndPoints - 1];
			y			= -mRadius * mpSinPoints[mEndPoints - 1];
		}
		else if((mDialValue >= mpValues[i]) && (mDialValue < mpValues[i + 1])){
			found		= TRUE;

			delta		= mpPoints[i + 1] - mpPoints[i];
			if(delta > 0.0F){
				delta -= (2*PI);
			}
			
			slope			= delta / (mpValues[i + 1] - mpValues[i]);
			deflection	= (float)(mpPoints[i] + (slope * (mDialValue - mpValues[i])));

			if(deflection < -PI){
				deflection += (2*PI);
			}
			else if(deflection > PI){
				deflection -= (2*PI);
			}

         mlSinCos (&trig, deflection);
			cosDeflection	= trig.cos;
			sinDeflection	= trig.sin;

			x	= mRadius * cosDeflection;
			y	= -mRadius * sinDeflection;
		}

		if(found) {
			mpCanvas->Line(0.0F, 0.0F, x, -y);
		}
		else {
			i++;
		}

	} while((!found) && (i < mEndPoints));
}