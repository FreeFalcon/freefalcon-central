#include <windows.h>
#include "cpcb.h"
#include "flightData.h"
#include "cpgauge.h"


void CBAOAExec(void * pObject){
	CPGauge* pCPGauge;

	pCPGauge					= (CPGauge*) pObject;
	pCPGauge->mCurrentVal = cockpitFlightData.alpha;
}

void CBAOADisplay(void * pObject){
	CPGauge* pCPGauge;
	int		y, x1, x2;

	pCPGauge	= (CPGauge*) pObject;

	x1 = pCPGauge->mx + pCPGauge->mpParent->mx;
	x2 = pCPGauge->mWidth + x1;
	y	= pCPGauge->my + (pCPGauge->mHeight/2) + pCPGauge->mpParent->my - 3; //-3 VWFKLUDGE

	pCPGauge->DrawTape(pCPGauge->mCurrentVal, pCPGauge->mx, pCPGauge->my, FALSE);
	pCPGauge->mpCPRenderer->StartFrame();
	pCPGauge->mpCPRenderer->SetColor(0x0000585e);
	pCPGauge->mpCPRenderer->Render2DLine(x1, y, x2, y);
	pCPGauge->mpCPRenderer->FinishFrame();
}
