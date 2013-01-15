#include "stdhdr.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "mfd.h"
#include "rwr.h"
#include "hud.h"
#include "Graphics\Include\render2d.h"
#include "Graphics\Include\renderow.h"
#include "cpmanager.h"
#include "aircrft.h"
#include "lantirn.h"

extern RECT VirtualMFD[OTWDriverClass::NumPopups + 1];
ViewportBounds	hudViewportBounds;

void OTWDriverClass::DoPopUps(void)
{
int i;

   for (i=0; i<NumPopups; i++)
   {
      /*
      if (popupHas[i] == 2)
      {
         // Draw RWR Here
      }
      else
      */
      if (popupHas[i] >= 0)
      {
//         OTWImage->Compose (MfdDisplay[popupHas[i]]->image, &(VirtualMFD[NumPopups]), &(VirtualMFD[i]));
      }
   }
}



void OTWDriverClass::Draw2DHud(void)
{
    int oldFont;
    if (theLantirn && theLantirn->IsFLIR()) {
#if DO_HIRESCOCK_HACK
	if(!gDoCockpitHack && (mOTWDisplayMode == ModeHud || mOTWDisplayMode == ModePadlockEFOV ||
	    mOTWDisplayMode == Mode2DCockpit && pCockpitManager)) {
#else
	    if(mOTWDisplayMode == ModeHud || mOTWDisplayMode == ModePadlockEFOV ||
		mOTWDisplayMode == Mode2DCockpit && pCockpitManager) {
#endif		
		if(pCockpitManager->GetViewportBounds(&hudViewportBounds,BOUNDS_HUD))
		{
		    theLantirn->DisplayInit (renderer->GetImageBuffer());
		    renderer->FinishFrame();
		    theLantirn->GetDisplay()->SetViewport(hudViewportBounds.left,
			hudViewportBounds.top,
			hudViewportBounds.right,
			hudViewportBounds.bottom);

			float temppitch=theLantirn->GetDPitch();
			theLantirn->SetDPitch(temppitch+(((60.0f*DTR)-renderer->GetFOV())/8.0f));  //Wombat778 10-18-2003 added to compensate for changed FOV


		    theLantirn->SetFOV (renderer->GetFOV()*(hudViewportBounds.right-hudViewportBounds.left)/2.0f);  //Wombat778 10-18-2003 changed 60.0f * DTR to getfov()
		    theLantirn->Display(theLantirn->GetDisplay());			
		    renderer->StartFrame();														//Wombat778 10-18-2003 set pitch back to the original.
			theLantirn->SetDPitch(temppitch);
			
		}
	    }
	    
	}
	renderer->SetColor(TheHud->GetHudColor());
#if DO_HIRESCOCK_HACK
	if(!gDoCockpitHack && (mOTWDisplayMode == ModeHud || mOTWDisplayMode == ModePadlockEFOV ||
		mOTWDisplayMode == Mode2DCockpit && pCockpitManager)) {
#else
	if(mOTWDisplayMode == ModeHud || mOTWDisplayMode == ModePadlockEFOV ||
		mOTWDisplayMode == Mode2DCockpit && pCockpitManager) {
#endif		
	    if(pCockpitManager->GetViewportBounds(&hudViewportBounds,BOUNDS_HUD))
	    {
		oldFont = VirtualDisplay::CurFont();
		ShiAssert( otwPlatform );		// If we don't have this we could pass NULL for TheHud target...
		if (TheHud->Ownship())
		    TheHud->SetTarget( TheHud->Ownship()->targetPtr );	
		else
		    TheHud->SetTarget( NULL);	

		//Wombat778 11-12-2003 Prevents hud from getting shifted incorrectly in 1.25 ratio modes in full screen hud mode

		if ((mOTWDisplayMode == ModeHud) && ((float) DisplayOptions.DispWidth / (float) DisplayOptions.DispHeight) == 1.25)		//Check for hud mode and 1.25 ratio
			//Wombat778 11-25-2003 Shift the viewport down by 1/16 (1/32 was not enough)
			renderer->SetViewport(hudViewportBounds.left,hudViewportBounds.top+((float) (hudViewportBounds.bottom-hudViewportBounds.top)*0.0625),hudViewportBounds.right,hudViewportBounds.bottom+((float)(hudViewportBounds.bottom-hudViewportBounds.top)*0.0625F));
		else
		
			renderer->SetViewport(hudViewportBounds.left,
				hudViewportBounds.top,
				hudViewportBounds.right,
				hudViewportBounds.bottom);

		
		
		// Should probably assert that hudViewportBounds.left = -hudViewportBounds.right
		// since we're assuming the HUD is horizontally centered in the display when it is active in 2D...
		TheHud->SetHalfAngle( (float)atan( hudViewportBounds.right * tan(renderer->GetFOV()/2.0f) ) * RTD );
		
		VirtualDisplay::SetFont(pCockpitManager->HudFont());
		if(TheHud->Ownship())
		{
		    TheHud->Display(renderer);
		}
		
		VirtualDisplay::SetFont(oldFont);
	    }
	}
	else {
	    ShiAssert( otwPlatform );		// If we don't have this we could pass NULL for TheHud target...
	    TheHud->SetTarget( TheHud->Ownship()->targetPtr );
	    
	    renderer->SetViewport(-0.4675F, 0.25F, 0.47F, -1.0F);
	    hudViewportBounds.left   = -0.4675F;
	    hudViewportBounds.top    =  0.25F;
	    hudViewportBounds.right  =  0.47F;
	    hudViewportBounds.bottom = -1.0F;
	    
	    TheHud->SetHalfAngle(15.1434F);
	    TheHud->Display(renderer);
	}

	renderer->SetColor(0xff00ff00);
}