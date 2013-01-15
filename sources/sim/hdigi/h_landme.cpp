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
#include "helo.h"

// KCK externals Ptdata functions
// RV - Biker - We don't use this ATM
//extern int GetFirstPt (int headerindex);
//extern int GetNextPt (int ptindex);
//extern void TranslatePointData (CampEntity e, int ptindex, float *x, float *y);
//extern int CheckHeaderStatus (CampEntity e, int index);

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
	GridIndex x, y;

	float groundAlt = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(), self->YPos() + self->YDelta());
	float selfAlt = self->ZPos() + self->offsetZ;

	switch (onStation) {

		case NotThereYet:
			break;

		case Arrived:
			jinkTime = SimLibElapsedTime + 30000;
			onStation = Landing;
			break;

		case Landing:
			if ((selfAlt >= (groundAlt - 0.25f)) && (selfAlt < groundAlt) || SimLibElapsedTime > jinkTime) {
				onStation = Landed;
				jinkTime = SimLibElapsedTime + (90 + rand()%90)*1000;
				self->SetFlag(ON_GROUND);
				break;
			}
			else if ((selfAlt >= (groundAlt - 3.5f) || SimLibElapsedTime > (jinkTime - 10000)) && (selfAlt < groundAlt)) {
				LevelTurn(0.0f, 0.0f, TRUE);
				MachHold(0.0f, -10.0f, FALSE);
			}
			else if ((selfAlt >= (groundAlt - 10.0f) || SimLibElapsedTime > (jinkTime - 15000)) && (selfAlt < groundAlt)) {
				LevelTurn(0.0f, 0.0f, TRUE);
				MachHold(0.0f, -3.5f, FALSE);
			}
			else {
				LevelTurn(0.0f, 0.0f, TRUE);
				MachHold(0.0f, 0.0f, FALSE);
			}

			if (self->OnGround()) {
				self->UnSetFlag(ON_GROUND);
			}
			
			// RV - Biker - Extend landing gear
			((DrawableBSP*)self->drawPointer)->SetSwitchMask(2, 1);
            break;

		case Landed:
			self->SetFlag(ON_GROUND);
			if (SimLibElapsedTime > jinkTime) {
				if (self->curWaypoint->GetWPAction() == WP_PICKUP) {
					onStation = PickUp;
					self->UnSetFlag(ON_GROUND);
				}
				else if (self->curWaypoint->GetWPAction() == WP_AIRDROP) {
					onStation = DropOff;
					self->UnSetFlag(ON_GROUND);
				}
				else
					onStation = Landed;			
			}
			// RV - Biker - Extend landing gear
			((DrawableBSP*)self->drawPointer)->SetSwitchMask(2, 1);
            break;

		// Load the airborne battalion.
		case PickUp:
			cargo = (Unit) self->curWaypoint->GetWPTarget();
			unit = (Unit)self->GetCampaignObject();
			
			if (cargo && unit) {
				unit->SetCargoId(cargo->Id());
				cargo->SetCargoId(unit->Id());
				cargo->SetInactive(1);
				unit->LoadUnit(cargo);
			}

			onStation = OnStation;
			break;

		// Unload the airborne battalion.
		case DropOff:
			cargo = (Unit) self->curWaypoint->GetWPTarget();
			unit = (Unit)self->GetCampaignObject();

			if (cargo && unit && unit->Cargo()) {
				unit->UnloadUnit();
				cargo->SetCargoId(FalconNullId);
				cargo->SetInactive(0);
				self->curWaypoint->GetWPLocation(&x,&y);
				cargo->SetLocation(x,y);
			}

			onStation = OnStation;
			break;

		case OnStation:
			break;

		case Departing:
      		break;
	}
}
