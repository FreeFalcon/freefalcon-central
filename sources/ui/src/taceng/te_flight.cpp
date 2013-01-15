///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Tactical Engagement - Robin Heydon
//
// Implements the user interface for the tactical engagement section
// of falcon 4.0
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <ddraw.h>
#include "unit.h"
#include "team.h"
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Listadt.h"
#include "objectiv.h"
#include "Campaign.h"
#include "campmap.h"
#include "campwp.h"
#include "campstr.h"
#include "squadron.h"
#include "feature.h"
#include "pilot.h"
#include "team.h"
#include "find.h"
#include "misseval.h"
#include "cmpclass.h"
#include "ui95_dd.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "AirUnit.h"
#include "uicomms.h"
#include "userids.h"
#include "classtbl.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "tac_class.h"
#include "te_defs.h"
#include "division.h"
#include "cmap.h"
#include "ui_cmpgn.h"
#include "ACSelect.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GetFlightStatus (Flight element, _TCHAR buffer[]);
void MakeIndividualATO(VU_ID flightID);
void GetMissionTarget(Package curpackage,Flight curflight,_TCHAR Buffer[]);
void SetupFlightSpecificControls (Flight flt);
void PickCampaignPlaneCB(long ID,short hittype,C_Base *control);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define MAX_TACTICAL_MISSIONS	1000

typedef struct
{
	double CampTime;
	VU_ID MissionID;
	_TCHAR StartTime[20];
	_TCHAR MissionType[20];
	_TCHAR Package[20];
	_TCHAR Status[20];
} FlightInfo;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static uint mission_count;
static FlightInfo flights[MAX_TACTICAL_MISSIONS];

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern VU_ID
	gPlayerFlightID,
	gCurrentFlightID,
	gSelectedFlightID,
	gActiveFlightID;

extern C_Map
	*gMapMgr;

#if 0
static long PlaneIDTable[4][4]=
{
	{ TAC_1_1,0,0,0,},
	{ TAC_2_1,TAC_2_2,0,0,},
	{ TAC_3_1,TAC_3_2,TAC_3_3,0,},
	{ TAC_4_1,TAC_4_2,TAC_4_3,TAC_4_4,},
};
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void pick_tactical_plane (long ID, short hittype, C_Base *control);
#endif
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void hookup_tactical_pick (C_Window *win)
{
	C_Button
		*button;

	button=(C_Button *)win->FindControl(CB_1_1);
	if(button)
		button->SetCallback(PickCampaignPlaneCB);

	button=(C_Button *)win->FindControl(CB_2_1);
	if(button)
		button->SetCallback(PickCampaignPlaneCB);

	button=(C_Button *)win->FindControl(CB_2_2);
	if(button)
		button->SetCallback(PickCampaignPlaneCB);

	button=(C_Button *)win->FindControl(CB_3_1);
	if(button)
		button->SetCallback(PickCampaignPlaneCB);

	button=(C_Button *)win->FindControl(CB_3_2);
	if(button)
		button->SetCallback(PickCampaignPlaneCB);

	button=(C_Button *)win->FindControl(CB_3_3);
	if(button)
		button->SetCallback(PickCampaignPlaneCB);

	button=(C_Button *)win->FindControl(CB_4_1);
	if(button)
		button->SetCallback(PickCampaignPlaneCB);

	button=(C_Button *)win->FindControl(CB_4_2);
	if(button)
		button->SetCallback(PickCampaignPlaneCB);

	button=(C_Button *)win->FindControl(CB_4_3);
	if(button)
		button->SetCallback(PickCampaignPlaneCB);

	button=(C_Button *)win->FindControl(CB_4_4);
	if(button)
		button->SetCallback(PickCampaignPlaneCB);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern MissionDataType MissionData[];

// KCK: These are used by the UI to create it's list boxes.
int IsValidMission (int dindex, int mission)
	{
	int					role = MissionData[mission].skill;
	UnitClassDataType	*uc;

	switch (mission)
		{
		case AMIS_NONE:
		case AMIS_SAD:				
		case AMIS_BAI:
		case AMIS_ABORT:
		case AMIS_ALERT:
		case AMIS_INTSTRIKE:
			return FALSE;
			break;
		case AMIS_STSTRIKE:
			uc = (UnitClassDataType*) (Falcon4ClassTable[dindex].dataPtr);
			if (uc->Scores[role] && (uc->Flags & VEH_STEALTH))
				return TRUE;
			break;
		case AMIS_PATROL:			
		case AMIS_RECONPATROL:	
		case AMIS_AIRCAV:
		case AMIS_SAR:    			
			// Helo only
			uc = (UnitClassDataType*) (Falcon4ClassTable[dindex].dataPtr);
			if (uc->Scores[role]) //  && 0)	// Need to check for helo types
				return TRUE;
			break;
		case AMIS_TRAINING:
		case AMIS_STRIKE:
		default:
			uc = (UnitClassDataType*) (Falcon4ClassTable[dindex].dataPtr);
			if (uc->Scores[role])
				return TRUE;
		}
	return FALSE;
	}

int IsValidTarget (Team team, int mission, CampEntity target)
	{
	if (mission < 0)
		return FALSE;
	if (!target && (MissionData[mission].target == AMIS_TAR_LOCATION || MissionData[mission].target == AMIS_TAR_NONE))
		return TRUE;
	else if (target && target->IsUnit() && ((Unit)target)->Real() && !target->IsSquadron() && MissionData[mission].target == AMIS_TAR_UNIT)
		{
		switch (mission)
			{
			case AMIS_HAVCAP:
			case AMIS_ESCORT:
			case AMIS_SEADESCORT:
				// Friendly air units only
				if (target->GetTeam() == team && target->IsFlight())
					return TRUE;
				break;
			case AMIS_INTERCEPT:
				// Enemy air units only
				if (target->IsFlight() && GetRoE(team,target->GetTeam(),ROE_AIR_FIRE) == ROE_ALLOWED)
					return TRUE;
				break;
			case AMIS_PRPLANCAS:
				// Enemy ground units only
				if (target->IsBattalion() && GetRoE(team,target->GetTeam(),ROE_GROUND_FIRE) == ROE_ALLOWED)
					return TRUE;
				break;
			case AMIS_SEADSTRIKE:
				// Enemy air defense units only
				if (target->IsBattalion() && ((Unit)target)->GetUnitNormalRole() == GRO_AIRDEFENSE && GetRoE(team,target->GetTeam(),ROE_GROUND_FIRE) == ROE_ALLOWED)
					return TRUE;
				break;
			case AMIS_ASW:
			case AMIS_ASHIP:
				if (target->IsTaskForce() && GetRoE(team,target->GetTeam(),ROE_GROUND_FIRE) == ROE_ALLOWED)
					return TRUE;
				break;
			case AMIS_AIRCAV:
				if (target->IsBattalion() && ((Unit)target)->GetSType() == STYPE_UNIT_AIRMOBILE)
					return TRUE;
			default:
				break;
			}
		}
	else if (target && target->IsObjective() && MissionData[mission].target == AMIS_TAR_OBJECTIVE)
		{
		switch (mission)
			{
			case AMIS_OCASTRIKE:
				// Enemy airbases, airstrips, radar, & CCC
				if (GetRoE(team,target->GetTeam(),ROE_GROUND_FIRE) &&
				   (target->GetType() == TYPE_AIRBASE || target->GetType() == TYPE_AIRSTRIP || target->GetType() == TYPE_RADAR || target->GetType() == TYPE_COM_CONTROL))
					return TRUE;
				break;
			case AMIS_INTSTRIKE:
				// Enemy bridges, production facilities, ports, depots, etc.
				if (GetRoE(team,target->GetTeam(),ROE_GROUND_FIRE) &&
				   (target->GetType() == TYPE_BRIDGE || target->GetType() == TYPE_CHEMICAL || target->GetType() == TYPE_DEPOT || target->GetType() == TYPE_FACTORY ||
				    target->GetType() == TYPE_NUCLEAR || target->GetType() == TYPE_PORT || target->GetType() == TYPE_POWERPLANT || target->GetType() == TYPE_RAIL_TERMINAL ||
				    target->GetType() == TYPE_REFINERY))
					return TRUE;
				break;
			case AMIS_STRIKE:
			case AMIS_DEEPSTRIKE:
			case AMIS_STRATBOMB:
			case AMIS_RECON:
			case AMIS_BDA:
				// Any enemy objective
				if (GetRoE(team,target->GetTeam(),ROE_GROUND_FIRE))
					return TRUE;
				break;
			default:
				break;
			}
		}
	return FALSE;
	}

int IsValidAction (int mission, int action)
	{
	switch (action)
		{
		case WP_NOTHING:	
		case WP_TAKEOFF:	
		case WP_ASSEMBLE:	
		case WP_POSTASSEMBLE:
		case WP_REFUEL:      	
		case WP_LAND:			
			return TRUE;
			break;
		case WP_REARM:     	
			break;
		case WP_PICKUP:
		case WP_AIRDROP:
			if (mission == AMIS_AIRCAV)
				return TRUE;
			break;
		case WP_STRIKE:
			if (mission == AMIS_TRAINING)
				return TRUE;
			// KCK: allow drop into the following routine -\v
		default:
			if (action == MissionData[mission].targetwp)
				return TRUE;
			break;
		}
	return FALSE;
	}

int IsValidEnrouteAction (int mission, int action)
	{
	switch (action)
		{
		case WP_NOTHING:	
			return TRUE;
			break;
		default:
			if (action == MissionData[mission].routewp)
				return TRUE;
			break;
		}
	return FALSE;
	}

// This is pretty hackish, but hey.. it works
int GetMissionFromTarget (Team team, int dindex, CampEntity target)
	{
	if (target && target->IsObjective())
		{
		if (!GetRoE(team,target->GetTeam(),ROE_GROUND_FIRE))
			target = NULL;
		else if ((target->GetType() == TYPE_AIRBASE || target->GetType() == TYPE_AIRSTRIP || target->GetType() == TYPE_RADAR || target->GetType() == TYPE_COM_CONTROL) &&
				 IsValidMission(dindex,AMIS_OCASTRIKE))
			return AMIS_OCASTRIKE;
		else if ((target->GetType() == TYPE_BRIDGE || target->GetType() == TYPE_CHEMICAL || target->GetType() == TYPE_DEPOT || target->GetType() == TYPE_FACTORY ||
				  target->GetType() == TYPE_NUCLEAR || target->GetType() == TYPE_PORT || target->GetType() == TYPE_POWERPLANT || target->GetType() == TYPE_RAIL_TERMINAL ||
				  target->GetType() == TYPE_REFINERY) &&
				 IsValidMission(dindex,AMIS_INTSTRIKE))
			return AMIS_INTSTRIKE;
		else if (IsValidMission(dindex,AMIS_STRIKE))
			return AMIS_STRIKE;
		target = NULL;
		}
	if (target && target->IsUnit())
		{
		if (!GetRoE(team,target->GetTeam(),ROE_GROUND_FIRE))
			{
			if (target->IsFlight() && IsValidMission(dindex,AMIS_HAVCAP))
				return AMIS_HAVCAP;
			else
				target = NULL;
			}
		else if (target->IsFlight() && IsValidMission(dindex,AMIS_INTERCEPT))
			return AMIS_INTERCEPT;
		else if (target->IsBattalion() && ((Unit)target)->GetUnitNormalRole() == GRO_AIRDEFENSE && IsValidMission(dindex,AMIS_SEADSTRIKE))
			return AMIS_SEADSTRIKE;
		else if (target->IsBattalion() && IsValidMission(dindex,AMIS_PRPLANCAS))
			return AMIS_PRPLANCAS;
		else if (target->IsTaskForce() && IsValidMission(dindex,AMIS_ASHIP))
			return AMIS_ASHIP;
		target = NULL;
		}
	if (!target)
		{
		if (IsValidMission(dindex,AMIS_BARCAP))
			return AMIS_BARCAP;
		if (IsValidMission(dindex,AMIS_FAC))
			return AMIS_FAC;
		if (IsValidMission(dindex,AMIS_ONCALLCAS))
			return AMIS_ONCALLCAS;
		if (IsValidMission(dindex,AMIS_AWACS))
			return AMIS_AWACS;
		if (IsValidMission(dindex,AMIS_JSTAR))
			return AMIS_JSTAR;
		if (IsValidMission(dindex,AMIS_TANKER))
			return AMIS_TANKER;
		if (IsValidMission(dindex,AMIS_ECM))
			return AMIS_ECM;
		if (IsValidMission(dindex,AMIS_AIRLIFT))
			return AMIS_AIRLIFT;
		if (IsValidMission(dindex,AMIS_RECONPATROL))
			return AMIS_RECONPATROL;
		}
	return 0;
	}
