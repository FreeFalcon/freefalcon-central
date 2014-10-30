#include <stdio.h>
#include <conio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <time.h>
#include "CmpGlobl.h"
#include "ListADT.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "ASearch.h"
#include "Path.h"
#include "Find.h"
#include "vutypes.h"
#include "Campaign.h"
#include "update.h"
#include "Unit.h"
#include "airunit.h"
#include "Gndunit.h"
#include "navunit.h"
#include "ATM.h"
#include "GTM.h"
#include "mission.h"
#include "team.h"
#include "tactics.h"
#include "AIInput.h"
#include "CUIEvent.h"
#include "MsgInc/RadioChatterMsg.h"
#include "MsgInc/CampWeaponFireMsg.h"
#include "MsgInc/AWACsMsg.h"
#include "MissEval.h"
#include "CmpClass.h"
#include "classtbl.h"
#include "Debuggr.h"
#include "Tacan.h"
#include "falcsess.h"
#include "aircrft.h"

// ============================================
// Defines and other nifty stuff
// ============================================

//#define DEATH_TIMEOUT_MS 240000 // Wait 4 minutes before removing a dead unit.
#define DEATH_TIMEOUT_MS 7200000 // Wait 2 hours before removing a dead unit.

#define AIRLIFT_SUPPLIES 3 // Amount of stuff that C-130 brings in.. ;-)
#define AIRLIFT_FUEL 3
#define AIRLIFT_REPLACEMENTS 3

extern void AircraftLaunch(Flight f);
extern void GoHome(Flight flight);

#ifdef DEBUG
extern int gSupplyFromAirlift[NUM_TEAMS];
extern int gFuelFromAirlift[NUM_TEAMS];
extern int gReplacmentsFromAirlift[NUM_TEAMS];
#endif

// ============================================
// Some usefull macros
// ============================================

// ============================================
// Unit Entity ADT - Private Implementation
// ============================================

// ================================
// Global variables
// ================================

int ProcessedCount;

#ifdef DEBUG
DWORD gAverageFlightDetectionTime = 0, gAverageBattalionDetectionTime = 0, gAverageFlightMoveTime = 0, gAverageBattalionMoveTime = 0, gAverageBrigadeMovetime = 0;
int gFlightDetects = 0, gBattalionDetects = 0, gFlightMoves = 0, gBattalionMoves = 0, gBrigadeMoves = 0;
#endif DEBUG

// ================================
// Global funtion Prototypes
// ================================

// ---------------------------------
// Global Function Definitions
// =================================

int UpdateUnit(Unit u, CampaignTime DeltaTime)
{
    // Check for movement

    //START_PROFILE("UU REMOVE");
    // sfr: real and father units never removed here
    // @todo remove
    if (u->IsDead() and not u->Real() and not u->Father())
    {
        // Check if our death timeout is up
        // Otherwise, keep this guy around for a while - for new events or other stuff
        if (TheCampaign.CurrentTime - u->GetLastCheck() > DEATH_TIMEOUT_MS)
        {
            vuDatabase->Remove(u);
        }

        return -1;
    }

    //STOP_PROFILE("UU REMOVE");

    //START_PROFILE("UU UNSET");
    u->UnsetChecked();
    //STOP_PROFILE("UU UNSET");

#define FIX_RESET_UNIT 1
    CampaignTime lastCheck = u->GetLastCheck();

    if (
#if FIX_RESET_UNIT
        (lastCheck == 0) or
#endif
        (TheCampaign.CurrentTime - lastCheck > u->UpdateTime())
    )
    {
        //START_PROFILE("UU SET");
        u->SetLastCheck(TheCampaign.CurrentTime);
        //STOP_PROFILE("UU SET");

        if (u->Real() and not u->GetRoster())
        {
            //START_PROFILE("UU KILL");
            u->KillUnit(); // Unit is out of vehicles, kill it off
            //STOP_PROFILE("UU KILL");
            return -1;
        }
        else
        {
            //START_PROFILE("UU MOVE");
            u->MoveUnit(u->GetMoveTime());
            //STOP_PROFILE("UU MOVE");
        }


        //START_PROFILE("UU WP");
        // In tactical engagement, we want to make sure battalions always have a waypoint,
        // So they can be reordered
        if (
            u->IsBattalion() and 
            FalconLocalGame->GetGameType() == game_TacticalEngagement and 
 not u->GetCurrentUnitWP()
        )
        {
            GridIndex x, y;
            u->GetLocation(&x, &y);
            u->AddUnitWP(x, y, 0, 0, 0, 0, WP_MOVEUNOPPOSED);
        }

        //STOP_PROFILE("UU WP");
    }

    //START_PROFILE("UU COMBAT");
    // Check for combat
    if (u->Real() and u->IsAggregate() and u->GetCombatTime() > u->CombatTime())
    {
        u->ChooseTarget();
        u->DoCombat();
    }

    //STOP_PROFILE("UU COMBAT");

    return 1;
}

// ========================================
// Detection and Combat Routines
// ========================================

// Let's see if (a) can see (e)
void DetectOneWay(CampEntity a, FalconEntity *e, int d, int *det, int *ran)
{
    int em /* ADDED BY S.G. TO GET THE SHOOTERS OBJECT TYPE */, am = a->GetMovementType();

    em = e->GetMovementType();

    if (a->CanDetect(e))
        *det = 1;

    // if (a->IsUnit() and a->GetAproxWeaponRange(em) >= d and a->GetWeaponRange(em) >= d) // REMOVED BY S.G.
    // WILL MAKE SURE BOTH OBJECTS ARE PLANE, SHOOTER HAS MORE THEN JUST GUN (1 NM) AND TARGET WITHIN AT LEAST 20 NM OF US
    //Cobra changed to d <= 100 from 37 (only 20 nm)
    if (a->IsUnit() and a->GetAproxWeaponRange(em) >= d and ((am == Air and em == Air and d <= 100 and a->GetWeaponRange(em) > 1) or a->GetWeaponRange(em, e) >= d)) // 2002-03-08 MODIFIED BY S.G. Added 'e' at the end of a->GetWeaponRange so we test the min/max weapon range against this guy
        *ran = 1;
}

int Detected(Unit u, FalconEntity *e, float *range)
{
    int udet, edet, uran, eran, retval = 0;

    // KCK: Someday, I should make this range squared - but it'd affect a lot of shit
    *range = Distance(u->XPos(), u->YPos(), e->XPos(), e->YPos()) / GRID_SIZE_FT;

    if (*range < 1.0F)
        // 2001-03-26 MODIFIED BY S.G. IF THE FIRST UNIT IS BELOW THE SECOND ONE AT ANY ALTITUDE SEPARATION), OR IF ABOVE WITH LESS THAN 5000 FEET ALTITUDE SEPARATION, RETURN 'DETECTION BASED ON WHO SAW WHO'
        // return ALL_DETECTION;
    {
        int det = 0;

        if (u->ZPos() - e->ZPos() > -5000.0f)
            det = ENEMY_SAME_HEX bitor ENEMY_IN_RANGE bitor ENEMY_DETECTED bitor FRIENDLY_IN_RANGE;

        if (e->ZPos() - u->ZPos() > -5000.0f)
            det or_eq ENEMY_SAME_HEX bitor ENEMY_IN_RANGE bitor FRIENDLY_IN_RANGE bitor FRIENDLY_DETECTED;

        if (det)
            return(det);

        // Need to say we're in range, even if it can't detect it because we are very close
        uran = eran = 1;
        udet = edet = 0;
    }
    else
        // END OF MODIFIED SECTION
        uran = eran = udet = edet = 0;

    DetectOneWay(u, e, FloatToInt32(*range), &udet, &uran);

    if (e->IsUnit())
        DetectOneWay((Unit)e, u, FloatToInt32(*range), &edet, &eran);
    else if (e->IsAirplane())
        edet = udet; // KCK hack- let players spot only what spots them

    if (edet)
        retval or_eq FRIENDLY_DETECTED;

    if (eran)
        retval or_eq FRIENDLY_IN_RANGE;

    if (udet)
        retval or_eq ENEMY_DETECTED;

    if (uran)
        retval or_eq ENEMY_IN_RANGE;

    return retval;
}

int DoCombat(CampBaseClass *att, FalconEntity *def)
{
    uchar* damageMods;
    MoveType defmt;
    short weapon[MAX_TYPES_PER_CAMP_FIRE_MESSAGE];
    uchar wcount[MAX_TYPES_PER_CAMP_FIRE_MESSAGE];
    int d, i, id = 255, shot = 0;
    float bonus = 1.0F;
    GridIndex   defx, defy, attx, atty;

    if ( not att or not def)
        return -1;

    // 2001-06-13 ADDED BY S.G. BEFORE WE CAN REALLY COMBAT, WE MUST SEE BE ABLE TO DETECT THE TARGET OURSELF BUT ONLY IF WE ARE A BATTALION
    // This is only called by aggregated UNITS so I safely call CanDetect from att as a unit against def
    // Since this is for SOJ, limit it to battalions...
    if (((UnitClass *)att)->IsBattalion() and not ((UnitClass *)att)->CanDetect(def))
    {
        att->StepRadar(0, 0, 1);
        return 0;
    }

    // END OF ADDED SECTION

    // We need to collect a weapon list, and send a firedOn message for each type
    def->GetLocation(&defx, &defy);
    att->GetLocation(&attx, &atty);
    d = FloatToInt32(Distance(attx, atty, defx, defy));
    damageMods = def->GetDamageModifiers();
    defmt = def->GetMovementType();

    if (def->IsFlight() and not ((Flight)def)->Moving())
        defmt = NoMove; // Aircraft on the ground bomb away

    memset(weapon, 0, sizeof(weapon));
    memset(wcount, 0, sizeof(wcount));

    // We may want to support Sim entities at some point, but for now, attacker is assumed to be a unit
    ShiAssert(att->IsUnit());

    // Step our radar if we're shoot'n at them flying thingys..
    if ( not def->OnGround())
    {
        if (att->IsAggregate())
        {
            att->StepRadar(1, 1, (float) d);

            if (att->GetRadarMode() == FEC_RADAR_AQUIRE)//me123
            {
                // Shortern our next combat interval if we're aquiring
                // 2002-03-22 MODIFIED BY S.G. Combat time will happen using the experience of the shooter as well (3 to 8 seconds)
                // ((Unit)att)->SetCombatTime(TheCampaign.CurrentTime - ((GROUND_COMBAT_CHECK_INTERVAL-rand()%3-2)*CampaignSeconds));
                ((Unit)att)->SetCombatTime(TheCampaign.CurrentTime - (((BattalionClass *)att)->CombatTime() - (rand() % 4 + 6 - (TeamInfo[att->GetOwner()]->airDefenseExperience >> 5)) * CampaignSeconds));
            }
        }
    }

    ((Unit)att)->CollectWeapons(damageMods, defmt, weapon, wcount, d);

    if ( not weapon[0])
        return 0; // We have no weapons to shoot

    // Apply combat related bonuses
    bonus = CombatBonus(att->GetTeam(), ((Unit)att)->GetUnitPrimaryObjID());

    // Unit type specific stuff
    if (att->IsFlight())
    {
        bonus *= AirExperienceAdjustment(att->GetCountry());
        ((Flight)att)->UseFuel(100);
    }
    else if (att->IsBattalion())
    {
        bonus *= ((Battalion)att)->AdjustForSupply();
        ((Battalion)att)->SetUnitFatigue(((Battalion)att)->GetUnitFatigue() + 1);
    }

    // Minimum bonus
    if (bonus < 0.01F)
        bonus = 0.01F;

    // Adjust by combat bonus
    for (i = 0; i < MAX_TYPES_PER_CAMP_FIRE_MESSAGE and weapon[i] and wcount[i]; i++)
    {
        wcount[i] = FloatToInt32(wcount[i] * bonus);

        if ( not wcount[i])
            wcount[i] = 1; // minimum of one shot, regardless of bonuses

        shot++;
    }

    if ( not shot)
        return 0;

    // Mark us as taking a shot (only vs other air)
    if (att->IsFlight() and def->IsFlight())
        ((Flight)att)->SetFired(1);

    // if (att->IsFlight() and def->Id() == ((Flight)att)->GetUnitMissionTargetID())
    // SetAtTarget((Flight)att);

    // Apply the damage
    if (def->IsCampaign())
    {
        if (((CampEntity)def)->IsAggregate())
        {
            FalconCampWeaponsFire *cwfm = new FalconCampWeaponsFire(def->Id(), FalconLocalGame);
            cwfm->dataBlock.shooterID = att->Id();
            memcpy(cwfm->dataBlock.weapon, weapon, sizeof cwfm->dataBlock.weapon);
            memcpy(cwfm->dataBlock.shots, wcount, sizeof cwfm->dataBlock.shots);
            ((CampBaseClass*)def)->ApplyDamage(cwfm, 0);
        }
        else
        {
            FireOnSimEntity(att, (CampBaseClass*)def, weapon, wcount, 255);
        }
    }
    else if (def->IsSim())
    {
        FireOnSimEntity(att, (SimBaseClass*)def, weapon[0]);
    }

    return shot;
}

// =============================
// Mission support routines
// =============================

// Unit within operation area, needs to take it's Waypoint's action
// This currently is only called if this waypoint has a flag set
// 2002-02-20 COMMENT BY S.G. It is called as well if the WP action is WP_REFUEL under some condition (see ResetCurrentWP).
int DoWPAction(Flight u)
{
    int       action = WP_NOTHING, speed;
    WayPoint w, pw;
    FalconRadioChatterMessage *msg;

    w = u->GetCurrentUnitWP(); // Find this unit's WP action

    if ( not w)
        return 0;

    action = w->GetWPAction();

    // Check Actions
    switch (action)
    {
        case WP_TAKEOFF:
        {
#ifdef KEV_DEBUG
            //noprint MonoPrint("Unit %d taking off at time %d / %d.\n", u->GetCampID(), Camp_GetCurrentTime(), w->GetWPArrivalTime());
#endif
            AircraftLaunch(u); // Tell UI
            // Rack up a mission
            Squadron sq = (Squadron) u->GetUnitSquadron();

            if (sq)
                sq->SendUnitMessage(u->Id(), FalconUnitMessage::unitStatistics, 0, ASTAT_MISSIONS, 1);
        }
        break;

        case WP_ASSEMBLE:

            // Radio Chatter messages
            if ( not u->GetUnitMissionID())
            {
                // Main flight
                msg = new FalconRadioChatterMessage(u->Id(), FalconLocalGame);
                msg->dataBlock.from = u->Id();
                msg->dataBlock.to = MESSAGE_FOR_PACKAGE;
                msg->dataBlock.voice_id = u->GetFlightLeadVoiceID();
                msg->dataBlock.message = rcPACKJOINED;
                msg->dataBlock.edata[0] = u->callsign_id;
                msg->dataBlock.edata[1] = u->GetFlightLeadCallNumber();
                FalconSendMessage(msg, FALSE);
                msg = new FalconRadioChatterMessage(u->Id(), FalconLocalGame);
                msg->dataBlock.from = u->Id();
                msg->dataBlock.to = MESSAGE_FOR_PACKAGE;
                msg->dataBlock.voice_id = u->GetFlightLeadVoiceID();
                msg->dataBlock.message = rcPACKDEPARTING;
                msg->dataBlock.edata[0] = u->callsign_id;
                msg->dataBlock.edata[1] = u->GetFlightLeadCallNumber();
                msg->dataBlock.time_to_play = 2 * CampaignSeconds;
                FalconSendMessage(msg, FALSE);
                FalconRadioChatterMessage *msg = new FalconRadioChatterMessage(u->Id(), FalconLocalGame);
                msg->dataBlock.from = u->Id();
                msg->dataBlock.to = MESSAGE_FOR_TEAM;
                msg->dataBlock.voice_id = u->GetFlightLeadVoiceID();
                msg->dataBlock.message = rcFLIGHTIN;
                msg->dataBlock.edata[0] = u->callsign_id;
                msg->dataBlock.edata[1] = u->GetFlightLeadCallNumber();
                //M.N. changed to 32767 -> flexibly use randomized value of max available eval indexes
                msg->dataBlock.edata[2] = 32767;
                msg->dataBlock.time_to_play = 4 * CampaignSeconds;
                FalconSendMessage(msg, FALSE);
            }
            else
            {
                // non main flight arriving at assembly point
                // KCK: We may want to skip this if the package has already departed
                msg = new FalconRadioChatterMessage(u->Id(), FalconLocalGame);
                msg->dataBlock.from = u->Id();
                msg->dataBlock.to = MESSAGE_FOR_PACKAGE;
                msg->dataBlock.voice_id = u->GetFlightLeadVoiceID();
                msg->dataBlock.message = rcPACKATJOIN;
                msg->dataBlock.edata[0] = -1;
                msg->dataBlock.edata[1] = -1;
                msg->dataBlock.edata[2] = u->callsign_id;
                msg->dataBlock.edata[3] = u->GetFlightLeadCallNumber();
                FalconSendMessage(msg, FALSE);
            }

            break;

        case WP_POSTASSEMBLE:
            // We're done with our task, so we can now be reassigned
            u->SetUnitPriority(0);
            break;

        case WP_REARM:
        case WP_REFUEL:
            u->SetBurntFuel(0);
            pw = w->GetNextWP();

            if ( not pw)
                GoHome((Flight)u);

            break;

        case WP_CASCP:
        {
            Flight fac = u->GetFACFlight();

            AircraftClass* lead = (AircraftClass*)u->GetComponentLead();

            // If we've got a FAC flight attached to us, send it a message
            // there has to be someone alive to talk
            if (fac and lead)
            {
                SendCallToAWACS(lead, rcFACCONTACT);
                // SendCallToAWACS(lead, rcFACREADY); // JPO removed so it doesn't collide with previous - could delay instead.
            }
        }
        break;

        case WP_LAND:
            // We're either landing mid-mission or as part of our mission -
            // Check if we're planning to take off again
            pw = w->GetNextWP();

            if ( not pw or ( not (w->GetWPFlags() bitand WPF_TAKEOFF) and pw->GetWPAction() not_eq WP_TAKEOFF))
            {
                UpdateSquadronStatus(u, TRUE, FALSE);
                return -1;
            }

            // Check if this was an airlift mission, and give us supplies, if so
            if (u->GetUnitMission() == AMIS_AIRLIFT)
            {
                int team = u->GetTeam();
                // RV - Biker - Make airlift supply dependent on package strength
                TeamInfo[team]->SetSupplyAvail(TeamInfo[team]->GetSupplyAvail() + AIRLIFT_SUPPLIES * u->CountUnitElements() / 3);
                TeamInfo[team]->SetFuelAvail(TeamInfo[team]->GetFuelAvail() + AIRLIFT_FUEL * u->CountUnitElements() / 3);
                TeamInfo[team]->SetReplacementsAvail(TeamInfo[team]->GetReplacementsAvail() + AIRLIFT_REPLACEMENTS * u->CountUnitElements() / 3);
#ifdef DEBUG
                gSupplyFromAirlift[team] += AIRLIFT_SUPPLIES;
                gFuelFromAirlift[team] += AIRLIFT_FUEL;
                gReplacmentsFromAirlift[team] += AIRLIFT_REPLACEMENTS;
#endif
            }

            if (pw->GetWPAction() == WP_TAKEOFF)
                u->SetCurrentUnitWP(pw);

            break;

        case WP_AIRDROP:
            if (u->Cargo())
            {
                // Drop off our cargo unit
                u->UnloadUnit();
            }

            break;

        case WP_PICKUP:

            // Pick up a unit
            if (u->GetUnitMission() == AMIS_AIRCAV and not u->Cargo())
                u->LoadUnit(NULL);

            break;

        default:
            break;
    }

    // Check Flags
    if ((w->GetWPFlags() bitand WPF_REPEAT) or (w->GetWPFlags() bitand WPF_REPEAT_CONTINUOUS))
    {
        speed = u->GetCruiseSpeed();

        // Check if we've been here long enough
        if (Camp_GetCurrentTime() > w->GetWPDepartureTime() and not (w->GetWPFlags() bitand WPF_REPEAT_CONTINUOUS))
        {
            // If so, go on to the next wp and adjust their times from now.
            u->SetCurrentUnitWP(w->GetNextWP());
            u->AdjustWayPoints();

            if (u->GetUnitMission() == AMIS_AWACS)
            {
                // AWACS off station
                msg = new FalconRadioChatterMessage(FalconNullId, FalconLocalGame);
                msg->dataBlock.from = u->Id();
                msg->dataBlock.to = MESSAGE_FOR_TEAM;
                msg->dataBlock.message = rcAWACSOFF;
                msg->dataBlock.voice_id = u->GetFlightLeadVoiceID();
                msg->dataBlock.edata[0] = u->callsign_id;
                msg->dataBlock.edata[1] = u->GetFlightLeadCallNumber();
                FalconSendMessage(msg, FALSE);
            }
        }
        else
        {
            // If not, restore previous WP and readjust times
            pw = w->GetPrevWP();
            u->SetCurrentUnitWP(pw);
            u->AdjustWayPoints();
        }
    }

    if (w->GetWPFlags() bitand WPF_CP)
    {
        if (u->GetUnitPriority() > 0 and u->GetUnitMission() == AMIS_AWACS)
        {
            // AWACs on station
            msg = new FalconRadioChatterMessage(FalconNullId, FalconLocalGame);
            msg->dataBlock.from = u->Id();
            msg->dataBlock.to = MESSAGE_FOR_TEAM;
            msg->dataBlock.message = rcAWACSON;
            msg->dataBlock.voice_id = u->GetFlightLeadVoiceID();
            msg->dataBlock.edata[0] = u->callsign_id;
            msg->dataBlock.edata[1] = u->GetFlightLeadCallNumber();
            FalconSendMessage(msg, FALSE);
        }
        else if (u->GetUnitMission() == AMIS_TANKER and not u->IsTacan())
        {
            // Tanker on station
            u->SetTacan(1);
        }

        u->SetUnitPriority(0); // We're just hanging out here... waiting for something to do.
    }

    if (w->GetWPFlags() bitand WPF_TARGET and not (w->GetWPFlags() bitand WPF_LAND) and not (w->GetWPFlags() bitand WPF_TAKEOFF) and not (w->GetWPFlags() bitand WPF_CP) and not (w->GetWPFlags() bitand WPF_REPEAT))
    {
        // Radio Chatter message
        FalconRadioChatterMessage *msg = new FalconRadioChatterMessage(u->Id(), FalconLocalGame);
        msg->dataBlock.from = u->Id();
        msg->dataBlock.to = MESSAGE_FOR_PACKAGE;
        msg->dataBlock.voice_id = u->GetFlightLeadVoiceID();
        msg->dataBlock.message = rcFLIGHTOFF;
        msg->dataBlock.edata[0] = u->callsign_id;
        msg->dataBlock.edata[1] = u->GetFlightLeadCallNumber();
        FalconSendMessage(msg, FALSE);
    }

    if (w->GetWPFlags() bitand WPF_DIVERT)
        return 1;
    else if (u->Diverted())
    {
        // We're back on our standard route
        u->SetDiverted(0);
        // KCK NOTE: We might want to check for abort here.
    }

    if (w->GetWPFlags() bitand WPF_TARGET or w->GetWPFlags() bitand WPF_TURNPOINT)
        u->SetUnitPriority(0); // We're done with our task, so we can now be reassigned

    return 1;
}

// ----------------------------
// Local Function Definitions
// ----------------------------

//
// Unit Damage and status functions
//

//
// Information gathering
//

// Returns seconds required to move
CampaignTime TimeToMove(Unit u, CampaignHeading h)
{
    GridIndex x, y;
    float cost;
    int speed;
    CampaignTime time;

    speed = u->GetUnitSpeed();

    if (h >= Here or speed < 1)
    {
        return u->GetMoveTime();
    }

    u->GetLocation(&x, &y);
    x += dx[h];
    y += dy[h];

    // This is our movement multiple
    // MLR - Bounds checking??? Derrrrr Timmmmmmeeeeeeey //Cobra 10/31/04 TJL
    if (x < 0)
    {
        x = 0;
    }

    if (y < 0)
    {
        y = 0;
    }

    if (x >= Map_Max_X)
    {
        x = (GridIndex)(Map_Max_X - 1);
    }

    if (y >= Map_Max_Y)
    {
        y = (GridIndex)(Map_Max_Y - 1);
    }

    cost = u->GetUnitMovementCost(x, y, h);
    // Adjust by our speed.
    time = (CampaignTime)TimeToArrive(cost, (float)speed);

    if (time < CampaignSeconds)
    {
        time = CampaignSeconds;
    }

    return time;
}

//
// Path Functions
//
/*
void UpdateLocation (GridIndex *x, GridIndex *y, Path path, int start, int end)
   {
   int               i;
   CampaignHeading   h;

   if (path == NULL)
      return;
   for (i=start; i<path->GetLength(); i++)
      {
      h = (CampaignHeading) path->GetDirection(i);
      if (i >= end or h>8)
         path->SetDirection(i, Here);
      else
         {
         *x += dx[h];
         *y += dy[h];
         }
      }
 if (end<path->GetLength())
 path->SetLength(end);
   }
*/
//
// WayPoint Actions
//

int EngageParent(Unit u, FalconEntity *e)
{
    Unit p;

    p = u->GetUnitParent();

    if ( not p or p->Engaged())
        return 0;

    p->SetTarget(e);
    p->SetEngaged(1);
    return 1;
}

//
// Aircraft support functions
//

