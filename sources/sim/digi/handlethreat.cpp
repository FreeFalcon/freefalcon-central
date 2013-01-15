#include "stdhdr.h"
#include "digi.h"
#include "aircrft.h"
#include "simveh.h"
#include "object.h"

/*
** Name: HandleThreat
** Description:
**		Take some sort of action against a threat that has been set via
**		the threat pointer.   The threat is presumed to be not our primary
**		target and was notified to us either thru a spike on radar, missile
**		or gun fire at us.
&&		Return TRUE if handling a threat
*/
BOOL DigitalBrain::HandleThreat( void )
{
	// no threat?
	if ( threatPtr == NULL )
		return FALSE;

	// decrement threat timer
	threatTimer -= SimLibMajorFrameTime;

	// we re-evaluate our threat after time has expired
	if ( threatTimer <= 0.0f )
	{
		// base it on range and threat's ata
		// NOTE: the current targetPtr should be the threat at this point
		if (!targetPtr ||
			(targetPtr != threatPtr) ||
			(targetPtr->BaseData()->IsSim() && !((SimBaseClass*)targetPtr->BaseData())->IsAwake()) ||
			targetData->range > 8.0F * NM_TO_FT || //me123 changes here
			(targetData->range > 5.0F * NM_TO_FT && targetData->ataFrom > 90.0f * DTR))
		{
			// we drop concern of this threat
			SetThreat(NULL);
			self->SetThreat( NULL, THREAT_NONE );
			threatTimer = 0.0F;
			return FALSE;
		}
		else {
			wvrCurrTactic = WVR_NONE;
			threatTimer = 10.0F;
		}

   }

	// just go into WvrEngage at this point -- needs more smarts
	WvrEngage();

	return TRUE;
}
