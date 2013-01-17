#include "stdafx.h"

#include "cpded.h"
#include "cpmanager.h"
#include "dispopts.h"
#include "aircrft.h"
#include "fack.h"
#include "otwdrive.h"
#include "Graphics\Include\renderow.h"
#include "simdrive.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;

extern bool g_bDEDSpacingFix;		//Wombat778 10-17-2003

#include "navsystem.h"
#include "aircrft.h"

//==================================================//
//	CPDed::~CPDed
//==================================================//

CPDed::~CPDed() {

}

//==================================================//
//	CPDed::CPDed
//==================================================//

CPDed::CPDed(ObjectInitStr *pobjectInitStr, DedInitStr *pdedInitStr) : CPObject(pobjectInitStr) {

	int		yOffset;
	float		halfWidth;
	float		halfHeight;

	mCycleBits				= 0xFFFF;
	//MI changed for ICP Stuff
	if(!g_bRealisticAvionics)
		yOffset					= (mDestRect.bottom - mDestRect.top) / 3; // 3 lines of DED text
	else
		yOffset					= (mDestRect.bottom - mDestRect.top) / 6; // 5 lines of DED text but spaced tighter

	mpLinePos[0].top		= mDestRect.top + 2;
	mpLinePos[0].left		= mDestRect.left;
	mpLinePos[0].bottom	= mpLinePos[0].top + yOffset;
	mpLinePos[0].right	= mDestRect.right;

	mpLinePos[1].top		= mpLinePos[0].bottom;
	mpLinePos[1].left		= mDestRect.left;
	mpLinePos[1].bottom	= mpLinePos[1].top + yOffset;
	mpLinePos[1].right	= mDestRect.right;

	mpLinePos[2].top		= mpLinePos[1].bottom;
	mpLinePos[2].left		= mDestRect.left;
	mpLinePos[2].bottom	= mpLinePos[2].top + yOffset;
	mpLinePos[2].right	= mDestRect.right;
	
	//MI added for ICP Stuff
	mpLinePos[3].top		= mpLinePos[2].bottom;
	mpLinePos[3].left		= mDestRect.left;
	mpLinePos[3].bottom		= mpLinePos[3].top + yOffset;
	mpLinePos[3].right		= mDestRect.right;

	mpLinePos[4].top		= mpLinePos[3].bottom;
	mpLinePos[4].left		= mDestRect.left;
	mpLinePos[4].bottom		= mpLinePos[4].top + yOffset;
	mpLinePos[4].right		= mDestRect.right;

	halfWidth				= (float) DisplayOptions.DispWidth * 0.5F;
	halfHeight				= (float) DisplayOptions.DispHeight * 0.5F;

	mLeft						= (mDestRect.left - halfWidth) / halfWidth;
   mRight					= (mDestRect.right - halfWidth) / halfWidth;
   mTop						= -(mDestRect.top - halfHeight) / halfHeight;
   mBottom					= -(mDestRect.bottom - halfHeight) / halfHeight;
   // JPO - let the cockpit designers decide
   mColor[0]					= pdedInitStr->color0;
   mColor[1]					= CalculateNVGColor(pdedInitStr->color0);
   mDedType = pdedInitStr->dedtype;
}

//==================================================//
//	ICPClass::Exec
//==================================================//

void CPDed::Exec(SimBaseClass*) { 

   // Check for UFC Failure
    switch(mDedType) {
    case DEDT_DED: // the real DED
	mpCPManager->mpIcp->Exec();
	break;
    case DEDT_PFL: // the PFL display
	mpCPManager->mpIcp->ExecPfl();
	break;
    }
}

//==================================================//
//	ICPClass::DisplayDraw
//==================================================//


void CPDed::DisplayDraw(void)
{
	int oldFont = VirtualDisplay::CurFont();
	// COBRA - RED- Pit Vibrations
	ViewportBounds	ViewPort;
	ViewPort.left=mLeft; 
	ViewPort.right=mRight;
	ViewPort.top=mTop;
	ViewPort.bottom=mBottom;
	OTWDriver.pCockpitManager->AddTurbulenceVp(&ViewPort);

	
	// Hmmm - what about PFL? Assume its on the same circuit?
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC && playerAC->mFaults && playerAC->mFaults->GetFault(FaultClass::ufc_fault)){
		return;
	}
	
	if(mpCPManager->mpIcp){
		if(!g_bRealisticAvionics){
			if(mDedType == DEDT_DED){
				if(!playerAC->HasPower(AircraftClass::UFCPower))
					return;
				//MI original code
				char line1[40] = "Line1";
				char line2[40] = "Line2";
				char line3[40] = "Line3";
				mpCPManager->mpIcp->GetDEDStrings(line1, line2, line3);
				
				OTWDriver.renderer->SetViewport(ViewPort.left, ViewPort.top, ViewPort.right, ViewPort.bottom);
				if (!OTWDriver.renderer->GetGreenMode())
					// JB 000819 Info from MI
					//OTWDriver.renderer->SetColor(0xFF009FFF);  
					OTWDriver.renderer->SetColor(mColor[0]);
				// JB 000819
				else
					//			 OTWDriver.renderer->SetColor(0xFF00A200);  
				OTWDriver.renderer->SetColor(mColor[1]);  
				VirtualDisplay::SetFont(mpCPManager->DEDFont());
				//MI to space them properly with the new DED
#if 0
				OTWDriver.renderer->ScreenText((float)mpLinePos[0].left + 1.0F, (float)mpLinePos[0].top, line1);
				OTWDriver.renderer->ScreenText((float)mpLinePos[1].left + 1.0F, (float)mpLinePos[1].top, line2);
				OTWDriver.renderer->ScreenText((float)mpLinePos[2].left + 1.0F, (float)mpLinePos[2].top, line3);	
#else
				OTWDriver.renderer->ScreenText((float)mpLinePos[0].left + 1.0F, (float)mpLinePos[0].top, line1);
				OTWDriver.renderer->ScreenText((float)mpLinePos[1].left + 1.0F, (float)mpLinePos[2].top, line2);
				OTWDriver.renderer->ScreenText((float)mpLinePos[2].left + 1.0F, (float)mpLinePos[4].top, line3);
#endif	

				VirtualDisplay::SetFont(oldFont);
			}
		}
		else
		{
			OTWDriver.renderer->SetViewport(ViewPort.left, ViewPort.top, ViewPort.right, ViewPort.bottom);
			if (!OTWDriver.renderer->GetGreenMode())
				OTWDriver.renderer->SetColor(mColor[0]); //FF009FFF 00? 00B 00G 00R
			else
				OTWDriver.renderer->SetColor(mColor[1]);  


			// JPO do this after the return...
			VirtualDisplay::SetFont(0);	//MI hardcoded because we NEED this font
			//we're a DED
			if(mDedType == DEDT_DED) 
			{
				if(!playerAC->HasPower(AircraftClass::UFCPower))
					return;
				
				//Wombat778 10-17-2003 Aeyes Ded spacing fix
				float stepx;
				if ((g_bDEDSpacingFix) || (DisplayOptions.DispWidth > 1024))	//Wombat778 12-12-2003 Changed to allow automatic selection of the fix at high resolutions
					stepx = VirtualDisplay::pFontSet->fontData[VirtualDisplay::pFontSet->fontNum][32].pixelWidth; 
				else 
					stepx = 5.0F;

				char buf[2];
				for(int j = 0; j < 5; j++)
				{
					float x = mpLinePos[j].left + 1.0F;
					float y = (float)mpLinePos[j].top;
					for(int i = 0; i < 26; i++)
					{
						buf[0] = mpCPManager->mpIcp->DEDLines[j][i];
						buf[1] = '\0';
						if(buf[0] != ' ' && mpCPManager->mpIcp->Invert[j][i] == 0)
							OTWDriver.renderer->ScreenText(x, y, buf,mpCPManager->mpIcp->Invert[j][i]);
						else if(mpCPManager->mpIcp->Invert[j][i] == 2)
							OTWDriver.renderer->ScreenText(x, y, buf,mpCPManager->mpIcp->Invert[j][i]);
						x += stepx;
					}
				}
			}
			//we're a PFL
			else  {
				if(!playerAC->HasPower(AircraftClass::PFDPower)){
					return;
				}
				
				//Wombat778 10-17-2003 Aeyes Ded spacing fix				
				float stepx;
				if ((g_bDEDSpacingFix) || (DisplayOptions.DispWidth > 1024)){
					//Wombat778 12-12-2003 Changed to allow automatic selection of the fix at high resolutions
					stepx = VirtualDisplay::pFontSet->fontData[VirtualDisplay::pFontSet->fontNum][32].pixelWidth; 
				}
				else {
					stepx = 5.0F;
				}


				char buf[2];
				for(int j = 0; j < 5; j++)
				{
					float x = mpLinePos[j].left + 1.0F;
					float y = (float)mpLinePos[j].top;
					for(int i = 0; i < 26; i++)
					{
						buf[0] = mpCPManager->mpIcp->PFLLines[j][i];
						buf[1] = '\0';
						if(buf[0] != ' ' && mpCPManager->mpIcp->PFLInvert[j][i] == 0)
							OTWDriver.renderer->ScreenText(x, y, buf,mpCPManager->mpIcp->PFLInvert[j][i]);
						else if(mpCPManager->mpIcp->PFLInvert[j][i] == 2)
							OTWDriver.renderer->ScreenText(x, y, buf,mpCPManager->mpIcp->PFLInvert[j][i]);
						x += stepx;
					}
				}
			}
		}
		VirtualDisplay::SetFont(oldFont);
	}
}






