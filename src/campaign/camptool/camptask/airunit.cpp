#include <stdio.h>
#include <conio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <math.h>
#include "CmpGlobl.h"
#include "F4Vu.h"
#include "ListADT.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "ASearch.h"
#include "Path.h"
#include "Find.h"
#include "APITypes.h"
#include "Campaign.h"
#include "ATM.h"
#include "CampList.h"
#include "campwp.h"
#include "update.h"
#include "loadout.h"
#include "campweap.h"
#include "airunit.h"
#include "tactics.h"
#include "Team.h"
#include "Feature.h"
#include "AIInput.h"
#include "CmpClass.h"
#include "CampMap.h"
#include "FalcSess.h"
#include "classtbl.h"

#include "debuggr.h"

// ============================================
// Externals
// ============================================

extern int GetArrivalSpeed (Unit u);
extern char	MissStr[AMIS_OTHER][16];
extern int MRX;
extern int MRY;

extern FILE
	*save_log,
	*load_log;

extern int
	start_save_stream,
	start_load_stream;

// =========================================
// Air Unit functions
// =========================================

AirUnitClass::AirUnitClass(int type) : UnitClass(type)
	{
	}

AirUnitClass::AirUnitClass(VU_BYTE **stream) : UnitClass(stream)
	{
	if (load_log)
	{
		fprintf (load_log, "%08x AirUnitClass ", *stream - start_load_stream);
		fflush (load_log);
	}

	}

AirUnitClass::~AirUnitClass (void)
	{
	// Nothing to do here
	}

int AirUnitClass::SaveSize (void)
	{
	return UnitClass::SaveSize();
	}

int AirUnitClass::Save (VU_BYTE **stream)
	{
	UnitClass::Save(stream);
	if (save_log)
	{
		fprintf (save_log, "%08x AirUnitClass ", *stream - start_save_stream);
		fflush (save_log);
	}
	return AirUnitClass::SaveSize();
	}

// event handlers
int AirUnitClass::Handle(VuFullUpdateEvent *event)
	{
	// copy data from temp entity to current entity
	return (UnitClass::Handle(event));
	}

MoveType AirUnitClass::GetMovementType (void)
	{
	if (GetAltitude() < LOW_ALTITUDE_CUTOFF)
		return LowAir;
	return Air;
	}

// This is the speed we're trying to go
int AirUnitClass::GetUnitSpeed (void)
	{
	if (GetType() == TYPE_FLIGHT)
		{
		if (GetUnitTactic() <= ATACTIC_ENGAGE_NAVAL)
			return GetCombatSpeed();
		else
			return GetArrivalSpeed(this);
		}
	else
		return 0;
	}

int AirUnitClass::IsHelicopter (void)
	{
	if (!(class_data->Flags & VEH_VTOL))
		return 0;
	if (GetSType() == STYPE_UNIT_ATTACK_HELO || GetSType() == STYPE_UNIT_TRANSPORT_HELO || GetSType() == STYPE_UNIT_RECON_HELO)
		return 1;
	return 0;
	}

int AirUnitClass::OnGround (void)
	{
	if(ZPos() >= 0.0F)
		return TRUE;
	else
		return FALSE;
	}
// =========================================
// Globals
// =========================================

int GetUnitScore (Unit u, MoveType mt)
	{
	if (!u)
		return 0;
	else if (u->IsPackage() || u->IsBrigade())
		{
		Unit	e;
		int		score = 0;
		e = u->GetFirstUnitElement();
		while (e)
			{
			score += GetUnitScore(e,mt);
			e = u->GetNextUnitElement();
			}
		return score;
		}
	else
		{
		if (u->GetUnitCurrentRole() == ARO_CA)
			{
	//		return u->GetUnitRoleScore(ARO_CA, CALC_TOTAL, USE_EXP | USE_VEH_COUNT);
			return u->class_data->HitChance[mt] * u->GetTotalVehicles();
			}
		else
			{
	//		return u->GetUnitRoleScore(ARO_CA, CALC_TOTAL, USE_EXP | USE_VEH_COUNT)/3;
			return u->class_data->HitChance[mt] * u->GetTotalVehicles() / 3;
			}
		}
	return 0;
	}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
