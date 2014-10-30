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
#include "vutypes.h"
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

extern int GetArrivalSpeed(const UnitClass *u);
extern char MissStr[AMIS_OTHER][16];
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

AirUnitClass::AirUnitClass(ushort type, VU_ID_NUMBER id) : UnitClass(type, id)
{
}


//AirUnitClass::AirUnitClass(VU_BYTE **stream, long size) : UnitClass(stream)
AirUnitClass::AirUnitClass(VU_BYTE **stream, long *size) : UnitClass(stream, size)
{
    if (load_log)
    {
        fprintf(load_log, "%08x AirUnitClass ", *stream - start_load_stream);
        fflush(load_log);
    }
}

AirUnitClass::~AirUnitClass(void)
{
    // Nothing to do here
}

int AirUnitClass::SaveSize(void)
{
    return UnitClass::SaveSize();
}

int AirUnitClass::Save(VU_BYTE **stream)
{
    UnitClass::Save(stream);

    if (save_log)
    {
        fprintf(save_log, "%08x AirUnitClass ", *stream - start_save_stream);
        fflush(save_log);
    }

    return AirUnitClass::SaveSize();
}

// event handlers
int AirUnitClass::Handle(VuFullUpdateEvent *event)
{
    // copy data from temp entity to current entity
    return (UnitClass::Handle(event));
}

MoveType AirUnitClass::GetMovementType(void)
{
    if (GetAltitude() < LOW_ALTITUDE_CUTOFF)
        return LowAir;

    return Air;
}

// This is the speed we're trying to go
int AirUnitClass::GetUnitSpeed() const
{
    if (GetType() == TYPE_FLIGHT)
    {
        if (GetUnitTactic() <= ATACTIC_ENGAGE_NAVAL)
        {
            return GetCombatSpeed();
        }
        else
        {
            return GetArrivalSpeed(this);
        }
    }

    return 0;
}

int AirUnitClass::IsHelicopter() const
{
    if ( not (class_data->Flags bitand VEH_VTOL))
    {
        return 0;
    }

    if (
        GetSType() == STYPE_UNIT_ATTACK_HELO or
        GetSType() == STYPE_UNIT_TRANSPORT_HELO or
        GetSType() == STYPE_UNIT_RECON_HELO
    )
    {
        return 1;
    }

    return 0;
}

int AirUnitClass::OnGround(void)
{
    if (ZPos() >= 0.0F)
        return TRUE;
    else
        return FALSE;
}
// =========================================
// Globals
// =========================================

int GetUnitScore(Unit u, MoveType mt)
{
    if ( not u)
        return 0;
    else if (u->IsPackage() or u->IsBrigade())
    {
        Unit e;
        int score = 0;
        e = u->GetFirstUnitElement();

        while (e)
        {
            score += GetUnitScore(e, mt);
            e = u->GetNextUnitElement();
        }

        return score;
    }
    else
    {
        if (u->GetUnitCurrentRole() == ARO_CA)
        {
            // return u->GetUnitRoleScore(ARO_CA, CALC_TOTAL, USE_EXP bitor USE_VEH_COUNT);
            return u->class_data->HitChance[mt] * u->GetTotalVehicles();
        }
        else
        {
            // return u->GetUnitRoleScore(ARO_CA, CALC_TOTAL, USE_EXP bitor USE_VEH_COUNT)/3;
            return u->class_data->HitChance[mt] * u->GetTotalVehicles() / 3;
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
