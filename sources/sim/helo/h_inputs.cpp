#include "stdhdr.h"
#include "helo.h"
#include "PilotInputs.h"
#include "hdigi.h"
#include "helimm.h"
#include "fcc.h"

void HelicopterClass::GatherInputs(void)
{
   hBrain->FrameExec (targetList, targetPtr);
	if (hBrain->IsSetFlag(BaseBrain::MslFireFlag))
		FCC->releaseConsent = TRUE;
	else
		FCC->releaseConsent = FALSE;
   fireGun     = hBrain->IsSetFlag(BaseBrain::GunFireFlag);

   // adjust for damage effects
   // if pctStrength is below zero we're just plane (sic) dying....
   if ( pctStrength > 0.0f  )
   {
      hBrain->pStick *= pctStrength;
      hBrain->rStick *= pctStrength;
      // hBrain->throtl *= pctStrength;
      hBrain->yPedal *= pctStrength;
   }
   else
   {
	   // bzzzt, thanks for playing.
       fireGun = FALSE;
       FCC->releaseConsent = FALSE;

	   // hm, about we just try pegging some values...
	   if ( hBrain->pStick < 0.0f )
	   		hBrain->pStick = -0.5f;
	   else
	   		hBrain->pStick = 0.5f;

	   if ( hBrain->rStick < 0.0f )
       	   		hBrain->rStick = -0.5f;
	   else
	   		hBrain->rStick = 0.5f;

	   if ( hBrain->yPedal < 0.0f )
	   		hBrain->yPedal = -0.5f;
	   else
	   		hBrain->yPedal = 0.5f;

	   hBrain->throtl = 0.0f;
   }
}
