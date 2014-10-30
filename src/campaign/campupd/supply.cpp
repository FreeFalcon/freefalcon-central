#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "CmpGlobl.h"
#include "ListADT.h"
#include "F4Vu.h"
#include "vutypes.h"
#include "Objectiv.h"
#include "Strategy.h"
#include "Unit.h"
#include "Find.h"
#include "Path.h"
#include "ASearch.h"
#include "Campaign.h"
#include "Update.h"
#include "f4vu.h"
#include "CampList.h"
#include "gtm.h"
#include "team.h"
#include "gndunit.h"
#include "gtmobj.h"
#include "AIInput.h"
#include "classtbl.h"
#include "Debuggr.h"

#define MAX_SUPPLIES 60000
#define MAX_SUPPLY_RATIO 0.5F

#define AUTOMATIC_SUPPLY 5
#define AUTOMATIC_REPLACEMENTS 1

#ifdef DEBUG
int gSupplyFromProduction[NUM_TEAMS];
int gSupplyFromAirlift[NUM_TEAMS];
int gSupplyFromOffensive[NUM_TEAMS];
int gFuelFromProduction[NUM_TEAMS];
int gFuelFromAirlift[NUM_TEAMS];
int gFuelFromOffensive[NUM_TEAMS];
int gReplacmentsFromProduction[NUM_TEAMS];
int gReplacmentsFromAirlift[NUM_TEAMS];
int gReplacmentsFromOffensive[NUM_TEAMS];
#endif

extern bool g_bPowerGrid;


// ====================
// Production functions
// ====================

// This function collects supply from all producers and adds it to the team's totals.
// This should be called by the Campaign Master only
int ProduceSupplies(CampaignTime deltaTime)
{
	Objective o;
	Objective po = NULL;
    Team who;
    float rate;
    int type;
    ulong supply[NUM_TEAMS] = {0}, fuel[NUM_TEAMS] = {0}, s, f, power;
    ulong replacements[NUM_TEAMS] = {0};
    MissionRequestClass mis;
    GridIndex x, y;

    // Produce supplies, fuel and reinforcements (factories and refineries)
    {
        VuListIterator myit(AllObjList);
        o = GetFirstObjective(&myit);

        while (o)
        {
            type = o->GetType();

            //Cobra Added Army, depot, and port per JimG
            if ((type == TYPE_FACTORY or type == TYPE_ARMYBASE or type == TYPE_DEPOT or type == TYPE_PORT)
               and o->GetObjectiveOldown() == o->GetOwner()) // Supply
            {
                if (g_bPowerGrid)
                {
                    o->GetLocation(&x, &y);
                    po = FindNearestFriendlyPowerStation(AllObjList, o->GetTeam(), x, y);

                    if (po)
                        power = po->GetObjectiveStatus();
                    else power = 0;
                }
                else power = 100;

                s = o->GetObjectiveDataRate() * power / 100;

                if (s)
                {
                    who = o->GetTeam();

                    supply[who] += FloatToInt32((s * DataRateModSup));          // A.S.  2001-12-09 DataRateModification for supply and fuel only
                    replacements[who] += FloatToInt32((s * DataRateModRepl));   // A.S.  2001-12-09 DataRateModification for replacements only

                    // *** old code ***
                    // supply[who] += s;
                    //     replacements[who] += s;


                    // Request an interdiction stike mission
                    mis.requesterID = o->Id();
                    o->GetLocation(&mis.tx, &mis.ty);
                    mis.vs = who;
                    mis.who = GetEnemyTeam(mis.vs);
                    mis.tot = Camp_GetCurrentTime() + rand() % deltaTime + 30 * CampaignMinutes;
                    mis.tot_type = TYPE_NE;
                    mis.targetID = o->Id();
                    mis.mission = AMIS_INTSTRIKE;
                    mis.roe_check = ROE_AIR_ATTACK;
                    mis.context = enemyProductionSource;
                    mis.priority = 0;
                    mis.RequestEnemyMission();

                    if (g_bPowerGrid)
                    {
                        // Request an interdiction stike mission against the power station
                        mis.requesterID = po->Id();
                        po->GetLocation(&mis.tx, &mis.ty);
                        mis.vs = who;
                        mis.who = GetEnemyTeam(mis.vs);
                        mis.tot = Camp_GetCurrentTime() + rand() % deltaTime + 30 * CampaignMinutes;
                        mis.tot_type = TYPE_NE;
                        mis.targetID = po->Id();
                        mis.mission = AMIS_INTSTRIKE;
                        mis.roe_check = ROE_AIR_ATTACK;
                        mis.context = enemyFuelSource;
                        mis.priority = 0;
                        mis.RequestMission();
                    }
                }
            }
            else if (type == TYPE_REFINERY and o->GetObjectiveOldown() == o->GetOwner()) // Fuel
            {
                if (g_bPowerGrid)
                {
                    o->GetLocation(&x, &y);
                    po = FindNearestFriendlyPowerStation(AllObjList, o->GetTeam(), x, y);

                    if (po)
                        power = po->GetObjectiveStatus();
                    else power = 0;
                }
                else power = 100;

                f = o->GetObjectiveDataRate() * power / 100;

                if (f)
                {
                    who = o->GetTeam();
                    fuel[who] += f;
                    // Request an interdiction stike mission
                    mis.requesterID = o->Id();
                    o->GetLocation(&mis.tx, &mis.ty);
                    mis.vs = who;
                    mis.who = GetEnemyTeam(mis.vs);
                    mis.tot = Camp_GetCurrentTime() + rand() % deltaTime + 30 * CampaignMinutes;
                    mis.tot_type = TYPE_NE;
                    mis.targetID = o->Id();
                    mis.mission = AMIS_INTSTRIKE;
                    mis.roe_check = ROE_AIR_ATTACK;
                    mis.context = enemyFuelSource;
                    mis.priority = 0;
                    mis.RequestMission();

                    if (g_bPowerGrid)
                    {
                        // Request an interdiction stike mission against the power station
                        mis.requesterID = po->Id();
                        po->GetLocation(&mis.tx, &mis.ty);
                        mis.vs = who;
                        mis.who = GetEnemyTeam(mis.vs);
                        mis.tot = Camp_GetCurrentTime() + rand() % deltaTime + 30 * CampaignMinutes;
                        mis.tot_type = TYPE_NE;
                        mis.targetID = po->Id();
                        mis.mission = AMIS_INTSTRIKE;
                        mis.roe_check = ROE_AIR_ATTACK;
                        mis.context = enemyFuelSource;
                        mis.priority = 0;
                        mis.RequestMission();
                    }
                }
            }

            // NOTE: IF WE WANT SPECIAL SUPPLY SOURCES (OFF-MAP SUPPLY SOURCES), ADD THE
            // BONUS HERE.
            // if (o->IsBonusSupplySource())
            // supply += 1000;

            o = GetNextObjective(&myit);
        }

    }
    // rates are per day - convert to this interval
    rate = (float)deltaTime / (float)CampaignDay;

    for (who = 0; who < NUM_TEAMS; who++)
    {
        int actionBonus = TeamInfo[who]->GetGroundAction()->actionType;

        // A.S. 2001-12-09  No Action Type Bonus for production of supply, fuel and replacements
        if (NoActionBonusProd)
            actionBonus = 1;

        // end added section

        supply[who] = AUTOMATIC_SUPPLY + FloatToInt32((supply[who] / 5) * rate * actionBonus);
        fuel[who] = AUTOMATIC_SUPPLY + FloatToInt32(fuel[who] * rate * actionBonus);
        replacements[who] = AUTOMATIC_REPLACEMENTS + FloatToInt32((replacements[who] / 40) * rate * actionBonus);

#ifdef DEBUG
        gSupplyFromProduction[who] += supply[who];
        gFuelFromProduction[who] += fuel[who];
        gReplacmentsFromProduction[who] += replacements[who];
#endif

        // Deplete unused extra supplies and move supplies to team supply pools
        supply[who] = (TeamInfo[who]->GetSupplyAvail() / 2) + supply[who];
        fuel[who] = (TeamInfo[who]->GetFuelAvail() / 2) + fuel[who];
        replacements[who] = TeamInfo[who]->GetReplacementsAvail() + replacements[who];

        if (supply[who] > MAX_SUPPLIES)
            supply[who] = MAX_SUPPLIES;

        TeamInfo[who]->SetSupplyAvail(supply[who]);

        if (fuel[who] > MAX_SUPPLIES)
            fuel[who] = MAX_SUPPLIES;

        TeamInfo[who]->SetFuelAvail(fuel[who]);
        TeamInfo[who]->SetReplacementsAvail(replacements[who]);
    }

    return 1;
}

// ==================
// Supply functions
// ==================

void AddSupply(Objective  o, int supply, int fuel)
{
    int s, f;
    WORD sup = o->static_data.local_data;

    s = LOBYTE(sup) + supply;

    if (s > 255)
        s = 255;

    f = HIBYTE(sup) + fuel;

    if (f > 255)
        f = 255;

    o->static_data.local_data = MAKEWORD(s, f);
}

// This sends supply from s to d, subtracting from losses and adding to it's supply traffic field
// Updates values to amount which actually made it. returns 0 if none made it.
int SendSupply(Objective s, Objective d, int *supply, int *fuel)
{
    Objective c;
    PathClass path;
    int i, l, n, loss, type;

    if ( not *supply and not *fuel)
        return 0;

    if (GetObjectivePath(&path, s, d, Foot, s->GetTeam(), PATH_MARINE) < 1)
        return 0;

    c = s;
    loss = 0;
    AddSupply(s, *supply / 10, *fuel / 10);

    for (i = 0; i < path.GetLength(); i++)
    {
        n = path.GetDirection(i);
        c = c->GetNeighbor(n);
        type = c->GetType();

        if (type == TYPE_ROAD or type == TYPE_INTERSECT or type == TYPE_RAILROAD or type == TYPE_BRIDGE)
        {
            AddSupply(c, *supply / 10, *fuel / 10);
            l = c->GetObjectiveSupplyLosses() + 2; // Automatic loss rate of 2% per objective
            *supply = *supply * (100 - l) / 100;
            *fuel = *fuel * (100 - l) / 100;
        }

        if ( not *supply and not *fuel)
            return 0;
    }

    return 1;
}

void SupplyUnit(Unit u, int sneed, int supply, int fneed, int fuel)
{
    Unit e;
    float sratio, fratio;

    if (u->IsBattalion() or u->IsSquadron())
    {
        if ( not supply and not fuel)
            return;

        // KCK: We can add supply and fuel directly now, since we're asserting all
        // entities are local to the host. Values will be updated via dirty data
        u->SupplyUnit(supply, fuel);
        // // Send the supply message
        // u->SendUnitMessage(u->Id(),FalconUnitMessage::unitSupply,supply,fuel,0);
    }
    else if (u->IsBrigade())
    {
        if (sneed > 0)
            sratio = (float)supply / (float)sneed;
        else
            sratio = 1.0F;

        if (fneed > 0)
            fratio = (float)fuel / (float)fneed;
        else
            fratio = 1.0F;

        e = u->GetFirstUnitElement();

        while (e)
        {
            sneed = e->GetUnitSupplyNeed(FALSE);
            fneed = e->GetUnitFuelNeed(FALSE);
            SupplyUnit(e, sneed, FloatToInt32(sneed * sratio), fneed, FloatToInt32(fneed * fratio));
            e = u->GetNextUnitElement();
        }
    }
}

// Supplies units - must be called by Campaign Master
int SupplyUnits(Team who, CampaignTime deltaTime)
{
    Objective o, s;
    Unit unit;
    int supply, fuel, replacements, gots, gotf, type;
    int sneeded = 0, fneeded = 0, rneeded = 0;
    float sratio, fratio, rratio;
    GridIndex x, y;
    MissionRequestClass mis;
    // A.S. begin additional variables 2001-12-09
    // A.S. We now distinguish between aircrafts and ground vehicles
    int rneeded_a = 0, rneeded_v = 0, repl_a = 0, repl_v = 0;
    float rratio_a, rratio_v, a_v_nratio, repl;  // A.S.
    float sqnbonus, lambda; // A.S.
    int repl_a_s = 0, repl_v_s = 0, repl_s = 0, repl_sa = 0, prob = 0; // A.S. debug variables
    // end added section

    if ( not TeamInfo[who] or not (TeamInfo[who]->flags bitand TEAM_ACTIVE))
        return 0;

    sratio = fratio = rratio = 0.0F;

    // A.S. begin, 2001-12-09.
    rratio_a = rratio_v = a_v_nratio = repl = lambda = 0.0F; // A.S.
    sqnbonus = RelSquadBonus;     // A.S. gives Sqn relative ( not ) more repl. than Bde.
    // end added section

    // zero supply values
    {
        VuListIterator objit(AllObjList);
        o = GetFirstObjective(&objit);

        while (o)
        {
            if (o->GetTeam() == who)
                o->static_data.local_data = 0;

            o = GetNextObjective(&objit);
        }
    }

    // Calculate needs
    {
        VuListIterator myit(AllUnitList);
        unit = GetFirstUnit(&myit);

        while (unit)
        {
            if (unit->GetTeam() == who and (unit->IsBattalion() or unit->IsSquadron()))
            {
                sneeded += unit->GetUnitSupplyNeed(FALSE);
                fneeded += unit->GetUnitFuelNeed(FALSE);
                rneeded += unit->GetFullstrengthVehicles() - unit->GetTotalVehicles();

                // A.S. begin
                if (unit->IsSquadron() and NoTypeBonusRepl) // A.S. extra calculation for squadrons
                {
                    rneeded_a += unit->GetFullstrengthVehicles() - unit->GetTotalVehicles();
                }

                // end added section
            }

            unit = GetNextUnit(&myit);
        }
    }


    // A.S. begin, 2001-12-09.
    if (NoTypeBonusRepl)
    {
        rneeded_v = rneeded - rneeded_a; //A.S. calculation for groud vehicles

        if (rneeded > 0)
            a_v_nratio = ((float)rneeded_a) / rneeded;
        else
            a_v_nratio = 1;

        repl = (float)TeamInfo[who]->GetReplacementsAvail();    // aggregate replacements available
        lambda =  a_v_nratio * sqnbonus;

        if (lambda > 1)
            lambda = 1;
    }

    // end added section


    // Calculate the maximum ratio of supply we can give out
    if (sneeded > 0)
        sratio = (float)TeamInfo[who]->GetSupplyAvail() / sneeded;

    if (sratio > MAX_SUPPLY_RATIO)
        sratio = MAX_SUPPLY_RATIO;

    if (fneeded > 0)
        fratio = (float)TeamInfo[who]->GetFuelAvail() / fneeded;

    if (fratio > MAX_SUPPLY_RATIO)
        fratio = MAX_SUPPLY_RATIO;

    if (rneeded > 0)
        rratio = (float)TeamInfo[who]->GetReplacementsAvail() / rneeded;

    if ( not NoTypeBonusRepl) // A.S. added if-condiion 2001-12-09
    {
        if (rratio > 0.25F)
            rratio = 0.25F;
    }

    // A.S. begin, 2001-12-09
    if (NoTypeBonusRepl)
    {
        if (rratio > MAX_SUPPLY_RATIO)
        {
            rratio = MAX_SUPPLY_RATIO;
        }

        if (rneeded_a > 0)
            rratio_a = repl / rneeded_a;
        else
            rratio_a = MAX_SUPPLY_RATIO;

        if (rneeded_v > 0)
            rratio_v = repl / rneeded_v;
        else
            rratio_v = MAX_SUPPLY_RATIO;

        if (rratio_a > MAX_SUPPLY_RATIO)
            rratio_a = MAX_SUPPLY_RATIO;

        if (rratio_v > MAX_SUPPLY_RATIO)
            rratio_v = MAX_SUPPLY_RATIO;

        if (repl == 0)   // to handle situations like 0/0 
        {
            rratio_a = 0;
            rratio_v = 0;
        }
    }

    // end added section


    // Supply units
    {
        VuListIterator myit(AllUnitList);
        unit = GetFirstUnit(&myit);

        while (unit)
        {
            // We only supply/repair Battalions and Squadrons
            if (
                unit->GetTeam() == who and (unit->IsBattalion() or unit->IsSquadron()) and 
                TheCampaign.CurrentTime - unit->GetLastResupplyTime() > unit->GetUnitSupplyTime()
            )
            {
                float typeBonus = 1.0F;

                if (unit->GetUnitCurrentRole() == GRO_ATTACK)
                    typeBonus = 6.0F;

                if (unit->IsSquadron())
                {
                    ((SquadronClass*)unit)->ReinforcePilots(2);
                    typeBonus = 2.0F;
                }

                // Add some randomness
                typeBonus += ((rand() % 50) - 25.0F) / 100.0F;

                if (typeBonus < 0.0F)
                    typeBonus = 0.0F;

                supply = FloatToInt32(unit->GetUnitSupplyNeed(FALSE) * sratio * typeBonus);
                fuel = FloatToInt32(unit->GetUnitFuelNeed(FALSE) * fratio * typeBonus);

                // A.S.  2001-12-09. No Type Bonus for replacements. This helps fixing the bug that units can get more replacements than available.
                if (NoTypeBonusRepl)
                    typeBonus = 1.0F;

                // end added section

                replacements = FloatToInt32((unit->GetFullstrengthVehicles() - unit->GetTotalVehicles()) * rratio * typeBonus);

                // A.S. begin, 2001-12-09
                repl_s += replacements; // debug

                if (NoTypeBonusRepl)  // New code for distinguishing between aircrafts and ground vehicles
                {
                    if (unit->IsSquadron()) // this algorithm guarantees that no team can get more repl than available
                    {
                        prob = rand() % 100;
                        repl_a = FloatToInt32((unit->GetFullstrengthVehicles() - unit->GetTotalVehicles()) * (lambda) * rratio_a);

                        if (TeamInfo[who]->GetReplacementsAvail() >= 1 and prob < 51 and (((float)unit->GetTotalVehicles()) / unit->GetFullstrengthVehicles() <= 0.8F))  // rounding up with probability 0.5
                            repl_a = FloatToInt32((float) ceil((unit->GetFullstrengthVehicles() - unit->GetTotalVehicles()) * (lambda) * rratio_a));

                        if (repl_a > 0)
                        {
                            TeamInfo[who]->SetReplacementsAvail(TeamInfo[who]->GetReplacementsAvail() - repl_a);
                            unit->ChangeVehicles(min(repl_a, TeamInfo[who]->GetReplacementsAvail()));
                            repl_a_s += min(repl_a, TeamInfo[who]->GetReplacementsAvail()); // debug
                        }
                    }
                    else
                    {
                        prob = rand() % 100;
                        repl_v = FloatToInt32((unit->GetFullstrengthVehicles() - unit->GetTotalVehicles()) * (1 - lambda) * rratio_v);

                        if (TeamInfo[who]->GetReplacementsAvail() > 12 and prob < 51 and (((float)unit->GetTotalVehicles()) / unit->GetFullstrengthVehicles() <= 0.85F))  // rounding up
                            repl_v = FloatToInt32((float) ceil((unit->GetFullstrengthVehicles() - unit->GetTotalVehicles()) * (1 - lambda) * rratio_v));

                        if (repl_v > 0)
                        {
                            TeamInfo[who]->SetReplacementsAvail(TeamInfo[who]->GetReplacementsAvail() - repl_v);
                            unit->ChangeVehicles(min(repl_v, TeamInfo[who]->GetReplacementsAvail()));
                            repl_v_s += min(repl_v, TeamInfo[who]->GetReplacementsAvail()); // debug
                        }
                    }

                    // prob = rand() % 100;
                }
                else
                {
                    if (replacements) // ++++++ old code begin ++++++
                    {
                        TeamInfo[who]->SetReplacementsAvail(TeamInfo[who]->GetReplacementsAvail() - replacements);

                        if (unit->IsSquadron())
                        {
                            repl_sa += replacements; // A.S. debug
                            TeamInfo[who]->SetReplacementsAvail(TeamInfo[who]->GetReplacementsAvail() - replacements);
                            unit->SetLastResupply(replacements);
                        }

                        unit->ChangeVehicles(replacements);
                    }
                } // end added section  (important: this section replaces( not ) the section marked with ++++++ old code ++++++ )

                if (fuel or supply)
                {
                    unit->GetLocation(&x, &y);
                    o = FindNearestFriendlyObjective(who, &x, &y, 0);

                    if (o)
                    {
                        s = FindNearestSupplySource(o);

                        if (s)
                        {
                            TeamInfo[who]->SetSupplyAvail(TeamInfo[who]->GetSupplyAvail() - supply);
                            TeamInfo[who]->SetFuelAvail(TeamInfo[who]->GetFuelAvail() - fuel);
                            gots = supply;
                            gotf = fuel;

                            if (SendSupply(s, o, &gots, &gotf))
                                SupplyUnit(unit, supply, gots, fuel, gotf);
                        }
                    }
                }

                unit->SetLastResupplyTime(TheCampaign.CurrentTime);
            }

            unit = GetNextUnit(&myit);
        }
    }

    // A.S. debug begin
    //if (NoTypeBonusRepl) {
    // if (who == 2 or who==6) {
    // FILE *deb;
    // deb = fopen("c:\\temp\\deb1.txt", "a");
    // fprintf(deb, "Team %2d  ReplaAvail = %3d  A_Needed = %3d  V_Needed %4d  Aircraft = %2d  Vehicle = %3d  TIME = %d\n", who, (int)repl, rneeded_a, rneeded_v, repl_a_s, repl_v_s, TheCampaign.CurrentTime );
    // fclose(deb);
    // }
    //}
    //else {
    // if (who == 2 or who==6) { // A.S. debug
    // FILE *deb;
    // deb = fopen("c:\\temp\\deb1.txt", "a");
    // fprintf(deb, "Team %2d  ReplaAvail = %3d  Needed = %3d bitor Repl_a = %2d repl_v = %3d bitor TIME = %d\n", who, TeamInfo[who]->GetReplacementsAvail(), rneeded, repl_sa, (repl_s-repl_sa) , TheCampaign.CurrentTime % CampaignHours );
    // fclose(deb);
    // }
    //}
    // A.S. debug end

    // Reset loss values, set supply values, and request missions
    {
        VuListIterator objit(AllObjList);
        o = GetFirstObjective(&objit);

        while (o)
        {
            if (o->GetTeam() == who)
            {
                supply = LOBYTE(o->static_data.local_data);
                fuel = HIBYTE(o->static_data.local_data);

                if (supply > 5 or fuel > 5)
                {
                    o->SendObjMessage(o->Id(), FalconObjectiveMessage::objSetSupply, (short)(supply), (short)(fuel), 0);
                    type = o->GetType();

                    if (type == TYPE_ROAD or type == TYPE_INTERSECT)
                    {
                        // Request an interdiction mission
                        mis.requesterID = o->Id();
                        o->GetLocation(&mis.tx, &mis.ty);
                        mis.vs = o->GetTeam();
                        mis.who = GetEnemyTeam(mis.vs);
                        mis.tot = Camp_GetCurrentTime() + (30 + rand() % 480) * CampaignMinutes;
                        mis.tot_type = TYPE_NE;
                        mis.targetID = FalconNullId;
                        mis.mission = AMIS_INT;
                        mis.roe_check = ROE_AIR_ATTACK;
                        mis.context = enemySupplyInterdictionZone;
                        mis.RequestMission();
                    }

                    // RV - Biker - Do something special for bridges
                    //if (type == TYPE_BRIDGE or type == TYPE_DEPOT or type == TYPE_PORT)
                    if (type == TYPE_DEPOT or type == TYPE_PORT)
                    {
                        // Request an interdiction strike mission
                        mis.requesterID = o->Id();
                        o->GetLocation(&mis.tx, &mis.ty);
                        mis.vs = o->GetTeam();
                        mis.who = GetEnemyTeam(mis.vs);
                        mis.tot = Camp_GetCurrentTime() + (30 + rand() % 480) * CampaignMinutes;
                        mis.tot_type = TYPE_NE;
                        mis.targetID = o->Id();
                        mis.mission = AMIS_INTSTRIKE;
                        mis.roe_check = ROE_AIR_ATTACK;

                        if (type == TYPE_DEPOT)
                            mis.context = enemySupplyInterdictionDepot;

                        if (type == TYPE_PORT)
                            mis.context = enemySupplyInterdictionPort;

                        mis.RequestMission();
                    }

                    if (type == TYPE_BRIDGE)
                    {
                        // Check distance to FLOT
                        GridIndex ox = 0, oy = 0;
                        o->GetLocation(&ox, &oy);
                        float dist = DistanceToFront(ox, oy);

                        if (dist >= 50.0f)
                        {
                            mis.requesterID = o->Id();
                            mis.tx = ox;
                            mis.ty = oy;
                            mis.vs = o->GetTeam();
                            mis.who = GetEnemyTeam(mis.vs);
                            mis.tot = Camp_GetCurrentTime() + (30 + rand() % 480) * CampaignMinutes;
                            mis.tot_type = TYPE_NE;
                            mis.targetID = o->Id();
                            mis.mission = AMIS_INTSTRIKE;
                            mis.roe_check = ROE_AIR_ATTACK;
                            mis.context = enemySupplyInterdictionBridge;
                            mis.RequestMission();
                        }
                    }
                }
            }

            o = GetNextObjective(&objit);
        }
    }
    return 1;
}

/*
// Supplies units - must be called by Campaign Master
int SupplyUnits (Team team)
 {
 Objective o,s;
 Unit p;
 int supply,fuel,gots,gotf,type,i,who;
 int sneeded[NUM_TEAMS],fneeded[NUM_TEAMS];
 float sratio[NUM_TEAMS],fratio[NUM_TEAMS];
 GridIndex x,y;
 MissionRequestClass mis;
 VuListIterator myit(AllParentList);
 VuListIterator objit(AllObjList);

 // zero supply values
 o = GetFirstObjective(&objit);
 while (o)
 {
 o->static_data.local_data = 0;
 o = GetNextObjective(&objit);
 }

 // zero record values
 for (i=0; i<NUM_TEAMS; i++)
 sneeded[i] = fneeded[i] = 0;

 // Calculate needs
 p = GetFirstUnit(&myit);
 while (p)
 {
 who = p->GetTeam();
 if (p->GetDomain() not_eq DOMAIN_AIR or p->GetType() == TYPE_SQUADRON)
 {
 sneeded[who] += p->GetUnitSupplyNeed(FALSE);
 fneeded[who] += p->GetUnitFuelNeed(FALSE);
 }
 p = GetNextUnit(&myit);
 }

 // Calculate what ratio of needed supply we're going to give out
 for (i=0; i<NUM_TEAMS; i++)
 {
 if (sneeded[i] > 0)
 sratio[i] = (float)TeamInfo[i]->supplyAvail / sneeded[i];
 else
 sratio[i] = 0.0F;
 if (sratio[i] > 1.0F)
 sratio[i] = 1.0F;
 if (fneeded[i] > 0)
 fratio[i] = (float)TeamInfo[i]->fuelAvail / fneeded[i];
 else
 fratio[i] = 0.0F;
 if (fratio[i] > 1.0F)
 fratio[i] = 1.0F;
 }

 // Supply units
 p = GetFirstUnit(&myit);
 while (p)
 {
 who = p->GetTeam();
 if (p->GetDomain() not_eq DOMAIN_AIR or p->GetType() == TYPE_SQUADRON)
 {
 supply = p->GetUnitSupplyNeed(FALSE) * sratio[who];
 fuel = p->GetUnitFuelNeed(FALSE) * fratio[who];
 if (supply > TeamInfo[who]->supplyAvail)
 supply = TeamInfo[who]->supplyAvail;
 if (fuel > TeamInfo[who]->fuelAvail)
 fuel = TeamInfo[who]->fuelAvail;
 if (fuel or supply)
 {
 p->GetLocation(&x,&y);
 o = FindNearestFriendlyObjective(p->GetTeam(),&x,&y,0);
 if (o)
 {
 s = FindNearestSupplySource(o);
 if (s)
 {
 TeamInfo[who]->supplyAvail -= supply;
 TeamInfo[who]->fuelAvail -= fuel;
 gots = supply;
 gotf = fuel;
 if (SendSupply(s,o,&gots,&gotf));
 SupplyUnit(p,supply,gots,fuel,gotf);
 }
 }
 }
 RepairUnit(p);
 }
 p = GetNextUnit(&myit);
 }

 // Calculate current supply percentages (for UI)
 int shave[NUM_TEAMS]={0},swant[NUM_TEAMS]={0},fhave[NUM_TEAMS]={0},fwant[NUM_TEAMS]={0};
 p = GetFirstUnit(&myit);
 while (p)
 {
 who = p->GetTeam();
 if (p->GetDomain() == DOMAIN_LAND or p->IsSquadron())
 {
 int ths = p->GetUnitSupplyNeed(TRUE);
 int thf = p->GetUnitFuelNeed(TRUE);
 shave[who] += ths;
 fhave[who] += thf;
 swant[who] += ths + p->GetUnitSupplyNeed(FALSE);
 fwant[who] += thf + p->GetUnitFuelNeed(FALSE);
 }
 p = GetNextUnit(&myit);
 }
 for (i=0; i<NUM_TEAMS; i++)
 {
 if (swant[i])
 TeamInfo[i]->currentStats.supplyLevel = shave[i]*100/swant[i];
 if (fwant[i])
 TeamInfo[i]->currentStats.fuelLevel = fhave[i]*100/fwant[i];
 }


 // Reset loss values, set supply values, and request missions
// CampEnterCriticalSection();
 o = GetFirstObjective(&objit);
 while (o)
 {
 supply = LOBYTE(o->static_data.local_data);
 fuel = HIBYTE(o->static_data.local_data);
 if (supply > 5 or fuel > 5)
 {
 o->SendObjMessage(o->Id(),FalconObjectiveMessage::objSetSupply,(short)(supply),(short)(fuel),0);
// o->SendObjMessage(o->Id(),FalconObjectiveMessage::objSetLosses,0,0,0);
 type = o->GetType();
 if (type == TYPE_ROAD or type == TYPE_INTERSECT)
 {
 // Request an interdiction mission
 mis.requesterID = o->Id();
 o->GetLocation(&mis.tx,&mis.ty);
 mis.vs = o->GetTeam();
 mis.who = GetEnemyTeam(mis.vs);
 mis.tot = Camp_GetCurrentTime() + (30+rand()%480)*CampaignMinutes;
 mis.tot_type = TYPE_NE;
 mis.targetID = FalconNullId;
 mis.mission = AMIS_INT;
 mis.roe_check = ROE_AIR_ATTACK;
 mis.context = enemySupplyInterdictionZone;
// mis.priority = (supply+fuel)/20;
 mis.RequestMission();
 }
 if (type == TYPE_BRIDGE or type == TYPE_DEPOT or type == TYPE_PORT)
 {
 // Request an interdiction strike mission
 mis.requesterID = o->Id();
 o->GetLocation(&mis.tx,&mis.ty);
 mis.vs = o->GetTeam();
 mis.who = GetEnemyTeam(mis.vs);
 mis.tot = Camp_GetCurrentTime() + (30+rand()%480)*CampaignMinutes;
 mis.tot_type = TYPE_NE;
 mis.targetID = o->Id();
 mis.mission = AMIS_INTSTRIKE;
 mis.roe_check = ROE_AIR_ATTACK;
 if (type == TYPE_BRIDGE)
 mis.context = enemySupplyInterdictionBridge;
 if (type == TYPE_DEPOT)
 mis.context = enemySupplyInterdictionDepot;
 if (type == TYPE_PORT)
 mis.context = enemySupplyInterdictionPort;
// mis.priority = (supply+fuel)/20;
 mis.RequestMission();
 }
 }
 o = GetNextObjective(&objit);
 }
// CampLeaveCriticalSection();
 return 1;
 }

*/
