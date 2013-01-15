#include <float.h>
#include "Graphics\Include\drawBSP.h"
#include "stdhdr.h"
#include "hdigi.h"
#include "simdrive.h"
#include "simveh.h"
#include "CampRwy.h"
#include "Find.h"
#include "campbase.h"
#include "camplist.h"
#include "campstr.h"
#include "unit.h"
#include "otwdrive.h"
#include "campwp.h"

// KCK externals Ptdata functions
extern int GetFirstPt (int headerindex);
extern int GetNextPt (int ptindex);
extern void TranslatePointData (CampEntity e, int ptindex, float *x, float *y);
extern int CheckHeaderStatus (CampEntity e, int index);

void HeliBrain::SetupLanding (void)
{
}

void HeliBrain::CleanupLanding(void)
{
}

void HeliBrain::FindRunway (void)
{
}

void HeliBrain::LandMe(void)
{
	Unit cargo, unit;
	GridIndex x,y;


   switch (onStation)
   {
      case NotThereYet:
      break;

	  // just got there, start decent
      case Arrived:

   			groundZ = OTWDriver.GetGroundLevel(self->XPos(), self->YPos());

			// next state
			onStation = Landing;

			LevelTurn (0.0f, 0.0f, TRUE);
    		AltitudeHold(holdAlt);
			MachHold(0.0f, 0.0F, FALSE);
//			MonoPrint( "HELO BRAIN Landing\n" );
      break;

      case Landing:
	  		if ( self->ZPos() >= groundZ - 5.0f || self->OnGround() )
			{
				rStick = 0.0f;
				pStick = 0.0f;
				throtl = 0.50f;
				onStation = Landed;
				jinkTime = SimLibElapsedTime + 30000;
//				MonoPrint( "HELO BRAIN Landed!\n" );
			}
			else
			{
				LevelTurn (0.0f, 0.0f, TRUE);
    			throtl = 0.00;
				MachHold(0.0f, 0.0F, FALSE);
			}
      break;

      case Landed:
			rStick = 0.0f;
			pStick = 0.0f;
			throtl = 0.5f;
			if ( SimLibElapsedTime > jinkTime )
			{
				if ( self->curWaypoint->GetWPAction() == WP_PICKUP )
					onStation = PickUp;
				else if ( self->curWaypoint->GetWPAction() == WP_AIRDROP )
					onStation = DropOff;
				else
					onStation = OnStation;
			}
      break;

      case PickUp:
			// Load the airborne battalion.
			cargo = (Unit) self->curWaypoint->GetWPTarget();
			unit = (Unit)self->GetCampaignObject();
			if (cargo && unit)
			{
				unit->SetCargoId(cargo->Id());
				cargo->SetCargoId(unit->Id());
				cargo->SetInactive(1);
				unit->LoadUnit(cargo);
			}
			rStick = 0.0f;
			pStick = 0.0f;
			throtl = 0.5f;
			onStation = OnStation;
      break;

      case DropOff:
			// Load the airborne battalion.
			cargo = (Unit) self->curWaypoint->GetWPTarget();
			unit = (Unit)self->GetCampaignObject();
			if (cargo && unit && unit->Cargo())
			{
				unit->UnloadUnit();
				cargo->SetCargoId(FalconNullId);
				cargo->SetInactive(0);
				self->curWaypoint->GetWPLocation(&x,&y);
				cargo->SetLocation(x,y);
			}
			rStick = 0.0f;
			pStick = 0.0f;
			throtl = 0.5f;
			onStation = OnStation;
      break;

      case OnStation:
			if ( self->OnGround() )
			{
				self->UnSetFlag( ON_GROUND );
			}
			rStick = 0.0f;
			pStick = 0.0f;
			throtl = 0.5f;
      break;

      case Departing:
      	break;
   }
}
