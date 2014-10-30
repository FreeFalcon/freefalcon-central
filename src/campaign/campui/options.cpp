
#include <stdio.h>
#include <string.h>
#include "cmpglobl.h"
#include "F4Vu.h"
#include "CampList.h"
#include "Find.h"
#include "PlayerOp.h"
#include "Unit.h"
#include "Team.h"
#include "Options.h"
#include "GndUnit.h"
#include "CmpClass.h"
#include "FalcSess.h"
#include "classtbl.h"
#include "Squadron.h"
#include "Supply.h"
#include "F4Find.h"

void ClearMissionLists(void);
void ClearEventList(void);
void SetInitialEvents(char* scenario);
// 2002-04-18 MN to read in bullseye reference location, FLOT sort direction and FLOT points maximum distance (for example for 2 front scenarios)
void ReadSpecialCampaignData(char* scenario);


// ========================
// Option Setting Functions
// ========================

uchar max_veh[5] = { 8, 12, 12, 16, 16 };

void AdjustCampaignOptions(void)
{
    CampEnterCriticalSection();
    AdjustExperienceLevels();
    AdjustForceRatios();
    // KCK: Don't muck with the mission list on a non-campaign thread.
    // I'd rather be certain to clear them when saving new games
    // ClearMissionLists();
    // NukeHistoryFiles();
    ClearEventList();
    CampLeaveCriticalSection();
    SetInitialEvents(TheCampaign.Scenario);
}

void ClearMissionLists(void)
{
    for (int i = 0; i < NUM_TEAMS; i++)
        TeamInfo[i]->atm->requestList->Purge();
}

void ClearEventList(void)
{
    TheCampaign.DisposeEventLists();
}

void AdjustExperienceLevels(void)
{
    int i;

    for (i = 0; i < NUM_TEAMS; i++)
    {
        if (GetTTRelations(FalconLocalSession->GetTeam(), i) == War)
        {
            TeamInfo[i]->airExperience = PlayerOptions.CampaignEnemyAirExperience() * 10 + 60;
            TeamInfo[i]->airDefenseExperience = PlayerOptions.CampaignEnemyGroundExperience() * 10 + 60;
        }
    }

    // 2002-04-16 MN place this here, so that the campaign data stuff
    // from trigger files is read in when starting a saved campaign, too..
    ReadSpecialCampaignData(TheCampaign.Scenario);
}

void AdjustSquadronPilotSkills(Squadron u)
{
    int i, skill;

    for (i = 0; i < PILOTS_PER_SQUADRON; i++)
    {
        skill = ((TeamInfo[u->GetOwner()]->airExperience - 60) / 10) + rand() % 3 - 1;

        if (skill > 4)
            skill = 4;

        if (skill < 0)
            skill = 0;

        u->GetPilotData(i)->pilot_skill_and_rating = 0x30 bitor skill;
        u->GetPilotData(i)->pilot_status = PILOT_AVAILABLE;
        u->GetPilotData(i)->missions_flown = 0;
    }
}

void AdjustForceRatios(void)
{
    Unit u;
    int eteam, pteam, prgu, ergu, prad, erad, prau, erau, prnu, ernu, t;

    pteam = FalconLocalSession->GetTeam();
    eteam = GetEnemyTeam(pteam);
    prgu = TheCampaign.GroundRatio;
    ergu = 4 - prgu;
    prad = TheCampaign.AirDefenseRatio;
    erad = 4 - prad;
    prau = TheCampaign.AirRatio;
    erau = 4 - prau;
    prnu = TheCampaign.NavalRatio;
    ernu = 4 - prnu;

    // Set the max vehicle slot by team
    for (t = 0; t < NUM_TEAMS; t++)
    {
        if (t == pteam)
        {
            TeamInfo[t]->max_vehicle[RCLASS_AIR] = max_veh[prau];
            TeamInfo[t]->max_vehicle[RCLASS_GROUND] = max_veh[prgu];
            TeamInfo[t]->max_vehicle[RCLASS_AIRDEFENSE] = max_veh[prad];
            TeamInfo[t]->max_vehicle[RCLASS_NAVAL] = max_veh[prnu];
        }
        else if (t == eteam)
        {
            TeamInfo[t]->max_vehicle[RCLASS_AIR] = max_veh[erau];
            TeamInfo[t]->max_vehicle[RCLASS_GROUND] = max_veh[ergu];
            TeamInfo[t]->max_vehicle[RCLASS_AIRDEFENSE] = max_veh[erad];
            TeamInfo[t]->max_vehicle[RCLASS_NAVAL] = max_veh[ernu];
        }
        else
        {
            TeamInfo[t]->max_vehicle[RCLASS_AIR] = VEHICLE_GROUPS_PER_UNIT;
            TeamInfo[t]->max_vehicle[RCLASS_GROUND] = VEHICLE_GROUPS_PER_UNIT;
            TeamInfo[t]->max_vehicle[RCLASS_AIRDEFENSE] = VEHICLE_GROUPS_PER_UNIT;
            TeamInfo[t]->max_vehicle[RCLASS_NAVAL] = VEHICLE_GROUPS_PER_UNIT;
        }
    }

    // Traverse all our real units, adjusting force strengths as necessary
    VuListIterator myit(AllUnitList);
    u = (Unit) myit.GetFirst();

    while (u)
    {
        // Fill the unit to capacity before chopping shit - NOTE: this shouldn't be callled after a
        // LoadCampaign, so these are fresh scenarios - Also, flights shouldn't be filled.
        u->SetLosses(0);

        if ( not u->IsFlight())
        {
            UnitClassDataType *uc = u->GetUnitClassData();

            for (int slot = 0; slot < VEHICLE_GROUPS_PER_UNIT; slot++)
                u->SetNumVehicles(slot, uc->NumElements[slot]);
        }

        if (u->GetDomain() == DOMAIN_LAND)
        {
            // KCK: This triggers recalculation of our final destination.
            // Only really usefull if our objective moves out from under us.
            u->SetTempDest(1);
        }

        // Now chop companies
        switch (u->GetRClass())
        {
            case RCLASS_AIR:
                if (u->GetTeam() == pteam)
                    ChopCompany(u, prau);
                else if (u->GetTeam() == eteam)
                    ChopCompany(u, erau);
                else
                    ChopCompany(u, DEFAULT_COMPANY_RATIO);

                if (u->IsSquadron())
                    AdjustSquadronPilotSkills((Squadron)u);

                break;

            case RCLASS_NAVAL:
                if (u->GetTeam() == pteam)
                    ChopCompany(u, prnu);
                else if (u->GetTeam() == eteam)
                    ChopCompany(u, ernu);
                else
                    ChopCompany(u, DEFAULT_COMPANY_RATIO);

                break;

            case RCLASS_AIRDEFENSE:
                if (u->GetTeam() == pteam)
                    ChopCompany(u, prad);
                else if (u->GetTeam() == eteam)
                    ChopCompany(u, erad);
                else
                    ChopCompany(u, DEFAULT_COMPANY_RATIO);

                break;

            default:
                if (u->GetTeam() == pteam)
                    ChopCompany(u, prgu);
                else if (u->GetTeam() == eteam)
                    ChopCompany(u, ergu);
                else
                    ChopCompany(u, DEFAULT_COMPANY_RATIO);

                break;
        }

        // Resupply to the team's supply level
        if (u->Real() and (u->GetDomain() == DOMAIN_LAND or u->GetDomain() == DOMAIN_SEA))
            u->SetUnitSupply(TeamInfo[u->GetTeam()]->startStats.supplyLevel);
        else if (u->IsSquadron())
        {
            UnitClassDataType* uc = u->GetUnitClassData();
            long fuel;
            float ratio;

            ShiAssert(uc);

            // Squadrons want enough fuel to load each plane SQUADRON_MISSIONS_PER_HOUR times per hour for 2 supply periods
            // fuel = (((uc->Fuel * u->GetTotalVehicles() * SQUADRON_MISSIONS_PER_HOUR*2*MIN_RESUPPLY)/60) * TeamInfo[u->GetTeam()]->startStats.fuelLevel)/100;
            fuel = ((u->GetUnitFuelNeed(FALSE) + u->GetUnitFuelNeed(TRUE)) * TeamInfo[u->GetTeam()]->startStats.fuelLevel) / 100;
            u->SetSquadronFuel(fuel * SUPPLY_PT_FUEL);

            // Now let's add our munititions
            ratio = (float)(TeamInfo[u->GetTeam()]->startStats.supplyLevel) / 100.0F;

            for (int i = 0; i < MAXIMUM_WEAPTYPES; i++)
                u->SetUnitStores(i, FloatToInt32(ratio * SquadronStoresDataTable[uc->SpecialIndex].Stores[i]));

            // Reset our stats
            for (int j = 0; j < ARO_OTHER; j++)
                ((Squadron)u)->SetRating(j, uc->Scores[j]);
        }

        u = (Unit) myit.GetNext();
    }
}

// KCK NOTE: Eliminate companies 3 and 4 depending on the force ratios
void ChopCompany(Unit u, int crating)
{
    int slot;

    for (slot = max_veh[crating]; slot < VEHICLE_GROUPS_PER_UNIT; slot++)
        u->SetNumVehicles(slot, 0);
}

