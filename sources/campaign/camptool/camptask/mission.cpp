#include <math.h>
#include "cmpglobl.h"
#include "camplib.h"
#include "listadt.h"
#include "find.h"
#include "path.h"
#include "asearch.h"
#include "campaign.h"
#include "mission.h"
#include "package.h"
#include "unit.h"
#include "atm.h"
#include "loadout.h"
#include "Team.h"
#include "AIInput.h"
#include "NoFly.h"
#include "MsgInc\MissionRequestMsg.h"
#include "CampMap.h"
#include "Time.h"
#include "CmpClass.h"
#include "classtbl.h"

#include "uiwin.h"
#include "F4Error.h"
extern CampaignTime TimeToArrive (float distance, float speed);
extern void ShowWPLeg (MapData md, GridIndex x, GridIndex y, GridIndex X, GridIndex Y, int color);
extern void ShowWP (MapData md, GridIndex X, GridIndex Y, int color);
extern void ShowPath (MapData md, GridIndex X, GridIndex Y, Path p, int color);
extern unsigned char ShowSearch;
extern int DColor;
extern int moveAlt;
extern int maxSearch;

#ifdef DEBUG
int counttanker = 0;
#endif

// Mission Modes
#define MMODE_TAKEOFF				0
#define MMODE_ENROUTE				1
#define MMODE_AT_ASSEMBLY			2
#define MMODE_INGRESS				3
#define MMODE_AT_BREAKPOINT			4
#define MMODE_IN_TARGET_AREA		5
#define MMODE_AT_TURNPOINT			6
#define MMODE_EGRESS				7
#define MMODE_AT_POSTASSEMBLY		8
#define MMODE_RETURN_TO_BASE		9
#define MMODE_LANDING				10

#define	MIN_DIST_FOR_INGRESS		50				// Minimum Distance To Target to fly at ingress altitude
#define MIN_DIST_FOR_CRUISE			30				// Minimum Distance To Front to fly at cruise altitude

#define	TT_AVERAGE					0
#define	TT_TOTAL					1
#define	TT_MAX						2

//#define CLIMB_RATIO					0.3F 2002-03-21 MN externalised g_fClimbRatio

#define MAX_ENEMY_CA_MISSIONS_FOR_HIGH_PROFILE	100	// Hard coded for now, may be variable later

// Some combined profile types
#define MPROF_STANDARD		(MPROF_LOW | MPROF_HIGH)
#define MPROF_FIXED			0

#ifdef USE_SH_POOLS
MEM_POOL MissionRequestClass::pool;
#endif

extern float g_fClimbRatio;
extern int g_nNoWPRefuelNeeded; // 2002-03-25 MN
extern bool g_bAddIngressWP; // 2002-03-25 MN

// ======================================
// MissionRequestClass
// ======================================

MissionRequestClass::MissionRequestClass (void)
	{
	targetID = FalconNullId;
	requesterID = FalconNullId;
	who = vs = 0;
	tot = 0;
	tx = ty = 0;
	flags = caps = 0;
	target_num = 255;
	tot_type = 0;
	mission = 0;
	priority = 0;
	speed = 0;
	match_strength = 0;
	aircraft = 0;
	context = 0;
	roe_check = 0;
	delayed = 0;	
	start_block = 0;
	final_block = 0;
	action_type = 0;
	memset(slots,255,4);
	min_to = -127;	
	max_to = 127;	
	}

MissionRequestClass::~MissionRequestClass (void)
	{
	// Nothing to do here.
	}

int MissionRequestClass::RequestMission (void)
	{
	if (!mission || priority < 0 || (vs && !GetRoE(who,vs,roe_check)))
		return -1;
	if ((TeamInfo[who]) && (TeamInfo[who]->atm) && (TeamInfo[who]->flags & TEAM_ACTIVE))
		{
		VuTargetEntity				*target = (VuTargetEntity*) vuDatabase->Find(TeamInfo[who]->atm->OwnerId());
		FalconMissionRequestMessage	*message = new FalconMissionRequestMessage(TeamInfo[who]->atm->Id(), vuLocalSessionEntity);
		GetPriority(this);
		message->dataBlock.request = *this;
		message->dataBlock.team = who;
		if (priority > 0)
			{
			FalconSendMessage(message,FALSE);			// KCK NOTE: Go ahead and let a few messages miss their target
			return 0;
			}
		else 
		    delete message; // JPO mem leak
		}
	return -1;
	}

int MissionRequestClass::RequestEnemyMission (void)
	{
	int		bonus = priority;
	if (!vs)
		return -1;
	for (who=1; who<NUM_TEAMS; who++)
		{
		if (TeamInfo[who])
			{
			priority = bonus;		// Reset to bonus priority;
			RequestMission();
			}
		}
	return 0;
	}

// =============================
// Globals
// =============================

MissionDataType MissionData [AMIS_OTHER] = 
 {	{ AMIS_NONE,		AMIS_TAR_NONE,		0,				MPROF_STANDARD,	TPROF_ATTACK,	TDESC_NONE,	WP_NOTHING,	WP_LAST,		0,		  0,	  0,  0,  0, 0,  0,  0, AMIS_NONE,		0,  0, 0, 0 },
	{ AMIS_BARCAP,		AMIS_TAR_LOCATION,	ARO_CA,			MPROF_HIGH,		TPROF_SEARCH,	TDESC_TTL,	WP_NOTHING,	WP_CAP,			100,	400,	200,  0, 15, 2,  5, 30, AMIS_NONE,	   15, 15, 0, AMIS_ADDAWACS | AMIS_NOTHREAT | AMIS_ADDTANKER | AMIS_DONT_COORD | AMIS_EXPECT_DIVERT | AMIS_NO_BREAKPT | AMIS_FLYALWAYS },
	{ AMIS_BARCAP2,		AMIS_TAR_LOCATION,	ARO_CA,			MPROF_HIGH,		TPROF_SEARCH,	TDESC_TTL,	WP_NOTHING,	WP_CAP,			100,	400,	200,  0, 15, 2,  5, 30, AMIS_NONE,	   15, 15, 0, AMIS_ADDAWACS | AMIS_NOTHREAT | AMIS_ADDTANKER | AMIS_DONT_COORD | AMIS_EXPECT_DIVERT | AMIS_NO_BREAKPT | AMIS_FLYALWAYS },
	{ AMIS_HAVCAP,		AMIS_TAR_UNIT,		ARO_CA,			MPROF_HIGH,		TPROF_SEARCH,	TDESC_ATA,	WP_ESCORT,	WP_CAP,			100,	400,	300,-60, 60, 2,  5, 30, AMIS_NONE,		1, 15, 0, AMIS_ADDAWACS | AMIS_NOTHREAT | AMIS_ADDTANKER | AMIS_NO_BREAKPT | AMIS_FLYALWAYS }, //AMIS_EXPECT_DIVERT | removed, don't divert HAVCAPs
	{ AMIS_TARCAP,		AMIS_TAR_LOCATION,	ARO_CA,			MPROF_HIGH,		TPROF_SEARCH,	TDESC_TAO,	WP_NOTHING,	WP_CAP,			100,	300,	200,-60, 15, 2,  5, 30, AMIS_NONE,	   30, 15, 0, AMIS_ADDAWACS | AMIS_HIGHTHREAT | AMIS_EXPECT_DIVERT | AMIS_NO_BREAKPT | AMIS_FLYALWAYS },
	{ AMIS_RESCAP,		AMIS_TAR_LOCATION,	ARO_CA,			MPROF_HIGH,		TPROF_SEARCH,	TDESC_TAO,	WP_ESCORT,	WP_CAP,			50,		100,	100,  0, 15, 2,  5, 30, AMIS_NONE,	   30, 15, 0, AMIS_ADDAWACS | AMIS_EXPECT_DIVERT | AMIS_NO_BREAKPT | AMIS_FLYALWAYS },
	{ AMIS_AMBUSHCAP,	AMIS_TAR_LOCATION,	ARO_CA,			MPROF_LOW,		TPROF_SEARCH,	TDESC_TTL,	WP_NOTHING,	WP_CAP,			2,		 10,	  7,  0, 15, 2,  5, 30, AMIS_NONE,	   30, 15, 0, AMIS_ADDAWACS | AMIS_NOTHREAT | AMIS_DONT_COORD | AMIS_EXPECT_DIVERT | AMIS_NO_BREAKPT | AMIS_FLYALWAYS },
	{ AMIS_SWEEP,		AMIS_TAR_LOCATION,	ARO_CA,			MPROF_HIGH,		TPROF_SWEEP,	TDESC_TTL,	WP_CA,		WP_CA,			100,	400,	200,  0,  0, 4,  5, 30, AMIS_NONE,		1, 30, 0, AMIS_ADDAWACS | AMIS_EXPECT_DIVERT | AMIS_NO_BREAKPT },
  	{ AMIS_ALERT,		AMIS_TAR_LOCATION,	ARO_CA,			MPROF_LOW,		TPROF_NONE,		TDESC_TAO,	WP_NOTHING,	WP_NOTHING,		0,		 10,	  0,  0,  0, 2,  0, 10, AMIS_NONE,		1,  0, 0, AMIS_NOTHREAT | AMIS_TARGET_ONLY | AMIS_EXPECT_DIVERT | AMIS_NO_BREAKPT | AMIS_FLYALWAYS },
	{ AMIS_INTERCEPT,	AMIS_TAR_UNIT,		ARO_CA,			MPROF_HIGH,		TPROF_TARGET,	TDESC_TAO,	WP_NOTHING,	WP_INTERCEPT,	100,	400,	200,  0,  0, 2,  0, 20, AMIS_NONE,		0,  0, 0, AMIS_IMMEDIATE | AMIS_ASSIGNED_TAR | AMIS_EXPECT_DIVERT | AMIS_FLYALWAYS },
	{ AMIS_ESCORT,		AMIS_TAR_UNIT,		ARO_CA,			MPROF_STANDARD,	TPROF_FLYBY,	TDESC_ATA,	WP_ESCORT,	WP_ESCORT,		50,		600,	200,-60,  0, 2, 10, 20, AMIS_NONE,		1,  0, 0, AMIS_ADDAWACS | AMIS_HIGHTHREAT | AMIS_MATCHSPEED | AMIS_NO_DIST_BONUS | AMIS_FLYALWAYS },
// 2001-06-30 MODIFIED BY S.G. SO SEAD STRIKES BEHAVES LIKE SEAD ESCORTS FOR EN ROUTE SAM THREAT
//	{ AMIS_SEADSTRIKE,	AMIS_TAR_UNIT,		ARO_SEAD,		MPROF_STANDARD,	TPROF_ATTACK,	TDESC_TAO,	WP_NOTHING,	WP_SEAD,		20,		120,	 40,  0,  0, 4, 10, 40, AMIS_ESCORT,	1,255, 0, AMIS_ADDECM | AMIS_ADDBDA | AMIS_AVOIDTHREAT | AMIS_ADDBARCAP | AMIS_MATCHSPEED | AMIS_HIGHTHREAT | AMIS_NO_TARGETABORT },
// Fixed by M.N. forgot to add the RP5 SEADSTRIKE line...
	{ AMIS_SEADSTRIKE,	AMIS_TAR_UNIT,		ARO_SEAD,		MPROF_STANDARD,	TPROF_ATTACK,	TDESC_ATA,	WP_SEAD,	WP_SEAD,		20,		120,	 40,  0,  0, 4, 10, 40, AMIS_ESCORT,	1,255, 0, AMIS_ADDECM | AMIS_ADDBDA | AMIS_AVOIDTHREAT | AMIS_ADDBARCAP | AMIS_MATCHSPEED | AMIS_HIGHTHREAT | AMIS_NO_TARGETABORT },
	{ AMIS_SEADESCORT,	AMIS_TAR_UNIT,		ARO_SEAD,		MPROF_STANDARD,	TPROF_FLYBY,	TDESC_ATA,	WP_SEAD,	WP_SEAD,		20,		120,	 40,-60,  0, 2, 10, 40, AMIS_NONE,		1,  0, 0, AMIS_ADDECM | AMIS_HIGHTHREAT | AMIS_MATCHSPEED | AMIS_NO_TARGETABORT | AMIS_NO_DIST_BONUS | AMIS_FLYALWAYS },
	{ AMIS_OCASTRIKE,	AMIS_TAR_OBJECTIVE,	ARO_S,			MPROF_STANDARD,	TPROF_ATTACK,	TDESC_TAO,	WP_NOTHING,	WP_STRIKE,		5,		120,	 80,  0,  0, 4, 10, 40, AMIS_ESCORT,	1,255, 0, AMIS_ADDBDA | AMIS_AVOIDTHREAT | AMIS_ADDSEAD | AMIS_ADDESCORT | AMIS_ADDBARCAP | AMIS_ADDOCASTRIKE | AMIS_MATCHSPEED | AMIS_NO_TARGETABORT },
	{ AMIS_INTSTRIKE,	AMIS_TAR_OBJECTIVE,	ARO_S,			MPROF_STANDARD,	TPROF_ATTACK,	TDESC_TAO,	WP_NOTHING,	WP_STRIKE,		5,		120,	 50,  0,  0, 4, 10, 40, AMIS_ESCORT,	1,255, 0, AMIS_ADDBDA | AMIS_AVOIDTHREAT | AMIS_ADDSEAD | AMIS_ADDBARCAP | AMIS_ADDOCASTRIKE | AMIS_MATCHSPEED | AMIS_NO_TARGETABORT },
	{ AMIS_STRIKE,		AMIS_TAR_OBJECTIVE,	ARO_S,			MPROF_STANDARD,	TPROF_ATTACK,	TDESC_TAO,	WP_NOTHING,	WP_STRIKE,		5,		120,	 80,  0,  0, 4, 10, 40, AMIS_ESCORT,	1,255, 0, AMIS_ADDAWACS | AMIS_ADDBDA | AMIS_AVOIDTHREAT | AMIS_ADDSEAD | AMIS_ADDESCORT | AMIS_ADDBARCAP | AMIS_ADDOCASTRIKE | AMIS_MATCHSPEED | AMIS_NO_TARGETABORT },
	{ AMIS_DEEPSTRIKE,	AMIS_TAR_OBJECTIVE,	ARO_S,			MPROF_STANDARD,	TPROF_ATTACK,	TDESC_TAO,	WP_NOTHING,	WP_STRIKE,		5,		120,	 80,  0,  0, 4, 10, 40, AMIS_ESCORT,	1,255, 0, AMIS_ADDAWACS | AMIS_ADDECM | AMIS_AVOIDTHREAT | AMIS_HIGHTHREAT | AMIS_ADDSEAD | AMIS_ADDBARCAP | AMIS_MATCHSPEED | AMIS_NO_TARGETABORT },
	{ AMIS_STSTRIKE,	AMIS_TAR_OBJECTIVE,	ARO_S,			MPROF_STANDARD,	TPROF_ATTACK,	TDESC_TAO,	WP_NOTHING,	WP_STRIKE,		5,		120,	 80,  0,  0, 4, 10, 40, AMIS_NONE,		1,255, VEH_STEALTH, AMIS_AVOIDTHREAT | AMIS_HIGHTHREAT | AMIS_ADDBARCAP | AMIS_MATCHSPEED | AMIS_NO_TARGETABORT },
	{ AMIS_STRATBOMB,	AMIS_TAR_OBJECTIVE,	ARO_SB,			MPROF_HIGH,		TPROF_ATTACK,	TDESC_TAO,	WP_NOTHING,	WP_BOMB,		200,	600,	300,  0,  0, 2, 10,120, AMIS_ESCORT,	1,255, 0, AMIS_ADDSEAD | AMIS_ADDESCORT | AMIS_ADDBARCAP | AMIS_ADDOCASTRIKE | AMIS_MATCHSPEED | AMIS_NO_TARGETABORT },
	{ AMIS_FAC,			AMIS_TAR_LOCATION,	ARO_FAC,		MPROF_LOW,		TPROF_LOITER,	TDESC_TAO,	WP_NOTHING,	WP_FAC,			5,		100,	 50,  0, 30, 1,  5, 60, AMIS_SWEEP,	   10, 20, 0, AMIS_ADDJSTAR | AMIS_AVOIDTHREAT | AMIS_ADDOCASTRIKE | AMIS_NOTHREAT | AMIS_ADDTANKER | AMIS_DONT_COORD | AMIS_NPC_ONLY | AMIS_FLYALWAYS },
	{ AMIS_ONCALLCAS,	AMIS_TAR_LOCATION,	ARO_GA,			MPROF_STANDARD,	TPROF_LOITER,	TDESC_TAO,	WP_NOTHING,	WP_CASCP,		2,		100,	 50,  0, 15, 2,  5, 60, AMIS_SWEEP,	   10, 20, 0, AMIS_ADDJSTAR | AMIS_AVOIDTHREAT | AMIS_ADDOCASTRIKE | AMIS_ADDFAC | AMIS_ADDBARCAP | AMIS_NOTHREAT | AMIS_ADDTANKER | AMIS_DONT_COORD | AMIS_NO_BREAKPT | AMIS_EXPECT_DIVERT | AMIS_FLYALWAYS },
	{ AMIS_PRPLANCAS,	AMIS_TAR_UNIT,		ARO_GA,			MPROF_STANDARD,	TPROF_TARGET,	TDESC_TAO,	WP_NOTHING,	WP_GNDSTRIKE,	2,		100,	 50,  0,  0, 2,  5, 60, AMIS_SWEEP,	   15, 20, 0, AMIS_ADDJSTAR | AMIS_AVOIDTHREAT | AMIS_ADDOCASTRIKE | AMIS_ADDBARCAP | AMIS_NOTHREAT | AMIS_NO_TARGETABORT | AMIS_FLYALWAYS },
	{ AMIS_CAS,			AMIS_TAR_LOCATION,	ARO_GA,			MPROF_STANDARD,	TPROF_TARGET,	TDESC_TAO,	WP_NOTHING,	WP_GNDSTRIKE,	2,		100,	 50,  0,  0, 2,  0, 20, AMIS_SWEEP,	    0,  0, 0, AMIS_ADDJSTAR | AMIS_AVOIDTHREAT | AMIS_ADDOCASTRIKE | AMIS_IMMEDIATE | AMIS_DONT_COORD | AMIS_ASSIGNED_TAR | AMIS_EXPECT_DIVERT | AMIS_FLYALWAYS },
	{ AMIS_SAD,			AMIS_TAR_LOCATION,	ARO_GA,			MPROF_STANDARD,	TPROF_SEARCH,	TDESC_ATA,	WP_SAD,		WP_SAD,			5,		200,	100,  0,  0, 2,  5, 60, AMIS_NONE,	   20, 30, 0, AMIS_ADDAWACS | AMIS_ADDJSTAR | AMIS_ADDBARCAP | AMIS_ADDTANKER | AMIS_DONT_COORD | AMIS_NO_BREAKPT },
	{ AMIS_INT,			AMIS_TAR_LOCATION,	ARO_GA,			MPROF_STANDARD,	TPROF_SEARCH,	TDESC_ATA,	WP_NOTHING,	WP_SAD,			5,		200,	100,  0,  5, 4,  5, 60, AMIS_NONE,		1, 30, 0, AMIS_ADDAWACS | AMIS_ADDJSTAR | AMIS_ADDBARCAP | AMIS_DONT_COORD | AMIS_NO_BREAKPT },
	{ AMIS_BAI,			AMIS_TAR_LOCATION,	ARO_GA,			MPROF_STANDARD,	TPROF_SEARCH,	TDESC_ATA,	WP_NOTHING,	WP_SAD,			5,		200,	100,  0, 15, 4,  0, 60, AMIS_NONE,		1, 30, 0, AMIS_AVOIDTHREAT | AMIS_DONT_COORD | AMIS_NO_BREAKPT },
	{ AMIS_AWACS,		AMIS_TAR_LOCATION,	ARO_AWACS,		MPROF_HIGH,		TPROF_LOITER,	TDESC_TAO,	WP_ELINT,	WP_ELINT,		300,	400,	400,  0,300, 1, 20,120, AMIS_HAVCAP,   50,120, 0, AMIS_ADDESCORT | AMIS_ADDSWEEP | AMIS_NOTHREAT | AMIS_AIR_LAUNCH_OK | AMIS_NO_DIST_BONUS | AMIS_FLYALWAYS },
	{ AMIS_JSTAR,		AMIS_TAR_LOCATION,	ARO_JSTAR,		MPROF_HIGH,		TPROF_LOITER,	TDESC_TAO,	WP_ELINT,	WP_ELINT,		300,	400,	400,  0,300, 1, 20,120, AMIS_HAVCAP,   50,120, 0, AMIS_ADDESCORT | AMIS_ADDSWEEP | AMIS_NOTHREAT | AMIS_AIR_LAUNCH_OK | AMIS_NO_DIST_BONUS | AMIS_FLYALWAYS },
	{ AMIS_TANKER,		AMIS_TAR_LOCATION,	ARO_TANK,		MPROF_HIGH,		TPROF_LOITER,	TDESC_TAO,	WP_TANKER,	WP_TANKER,		100,	300,	200,  0,300, 1, 20,120, AMIS_HAVCAP,   40,120, 0, AMIS_ADDESCORT | AMIS_ADDSWEEP | AMIS_NOTHREAT | AMIS_AIR_LAUNCH_OK | AMIS_NO_DIST_BONUS | AMIS_FLYALWAYS },
	{ AMIS_RECON,		AMIS_TAR_OBJECTIVE,	ARO_REC,		MPROF_LOW,		TPROF_ATTACK,	TDESC_TAO,	WP_NOTHING,	WP_RECON,		2,		600,	400,  0,  0, 2, 10, 40, AMIS_ESCORT,	1, 90, 0, AMIS_ADDBARCAP },
	{ AMIS_BDA,			AMIS_TAR_OBJECTIVE,	ARO_REC,		MPROF_STANDARD,	TPROF_ATTACK,	TDESC_TAO,	WP_NOTHING,	WP_RECON,		2,		200,	100,120,  0, 2, 10, 40, AMIS_NONE,		1,  0, 0, AMIS_AVOIDTHREAT | AMIS_MATCHSPEED | AMIS_NO_DIST_BONUS },
	{ AMIS_ECM,			AMIS_TAR_LOCATION,	ARO_ECM,		MPROF_HIGH,		TPROF_LOITER,	TDESC_TAO,	WP_JAM,		WP_JAM,			100,	500,	200,  0, 60, 1, 10, 60, AMIS_HAVCAP,   30, 60, 0, AMIS_ADDESCORT | AMIS_ADDSWEEP | AMIS_NOTHREAT | AMIS_NO_DIST_BONUS | AMIS_FLYALWAYS },
	{ AMIS_AIRCAV,		AMIS_TAR_UNIT,		ARO_TACTRANS,	MPROF_LOW,		TPROF_LAND,		TDESC_ATA,	WP_NOTHING,	WP_AIRDROP,		5,		 25,	  5,  0,  2, 4,  5, 40, AMIS_SWEEP,		1,  0, VEH_VTOL, AMIS_AVOIDTHREAT | AMIS_ADDSWEEP | AMIS_ADDESCORT | AMIS_MATCHSPEED | AMIS_DONT_USE_AC | AMIS_FUDGE_RANGE | AMIS_NO_TARGETABORT | AMIS_FLYALWAYS | AMIS_HIGHTHREAT /* KCK: TO ALLOW MORE TO FLY */ },
	{ AMIS_AIRLIFT,		AMIS_TAR_LOCATION,	ARO_TRANS,		MPROF_HIGH,		TPROF_LAND,		TDESC_TTL,	WP_NOTHING,	WP_LAND,		100,	300,	200,  0, 30, 1,  5, 40, AMIS_ESCORT,	1,  0, 0, AMIS_NOTHREAT | AMIS_MATCHSPEED | AMIS_DONT_USE_AC | AMIS_AIR_LAUNCH_OK | AMIS_NO_TARGETABORT | AMIS_NO_DIST_BONUS | AMIS_FLYALWAYS },
	{ AMIS_SAR,			AMIS_TAR_LOCATION,	ARO_TACTRANS,	MPROF_LOW,		TPROF_TARGET,	TDESC_TAO,	WP_NOTHING,	WP_RESCUE,		1,		 25,	  5,  0,  0, 1,  5, 40, AMIS_RESCAP,	1,  0, VEH_VTOL, AMIS_AVOIDTHREAT | AMIS_HIGHTHREAT | AMIS_ADDESCORT | AMIS_ADDBARCAP | AMIS_NO_TARGETABORT | AMIS_FLYALWAYS },
	{ AMIS_ASW,			AMIS_TAR_UNIT,		ARO_ASW,		MPROF_LOW,		TPROF_SEARCH,	TDESC_ATA,	WP_ASW,		WP_ASW,			5,		100,	 50,  0,  0, 1,  5, 40, AMIS_SWEEP,	   20,  0, VEH_NAVY, AMIS_ADDSWEEP | AMIS_MATCHSPEED | AMIS_NO_TARGETABORT | AMIS_NO_DIST_BONUS | AMIS_FLYALWAYS },
	{ AMIS_ASHIP,		AMIS_TAR_UNIT,		ARO_ASHIP,		MPROF_STANDARD,	TPROF_ATTACK,	TDESC_TAO,	WP_NOTHING,	WP_NAVSTRIKE,	5,		100,	 80,  0,  0, 2, 10, 40, AMIS_ESCORT,	1,  0, 0, AMIS_ADDESCORT | AMIS_ADDBARCAP | AMIS_MATCHSPEED | AMIS_NO_TARGETABORT | AMIS_NO_DIST_BONUS },
	{ AMIS_PATROL,		AMIS_TAR_LOCATION,	ARO_REC,		MPROF_STANDARD,	TPROF_TARGET,	TDESC_ATA,	WP_RECON,	WP_RECON,		100,	500,	 50,  0, 30, 1, 10, 60, AMIS_SWEEP,	   20, 60, VEH_NAVY, AMIS_ADDSWEEP | AMIS_MATCHSPEED | AMIS_NO_DIST_BONUS },
	{ AMIS_RECONPATROL,	AMIS_TAR_LOCATION,	ARO_REC,		MPROF_LOW,		TPROF_LOITER,	TDESC_TAO,	WP_RECON,	WP_RECON,		5,		 25,	  5,  0, 30, 2,  5, 60, AMIS_SWEEP,	   10, 60, VEH_VTOL | VEH_ARMY, AMIS_ADDSWEEP | AMIS_NPC_ONLY | AMIS_DONT_COORD | AMIS_TARGET_ONLY },
	{ AMIS_ABORT,		AMIS_TAR_LOCATION,	0,				MPROF_LOW,		TPROF_TARGET,	TDESC_NONE,	WP_NOTHING,	WP_NOTHING,		5,		500,	100,  0,  0, 0,  0, 60, AMIS_NONE,		0,  0, 0, AMIS_FLYALWAYS },
	{ AMIS_TRAINING,	AMIS_TAR_NONE,		0,				MPROF_STANDARD,	TPROF_ATTACK,	TDESC_NONE,	WP_NOTHING,	WP_NOTHING,		0,		  0,	  0,  0,  0, 0,  0,  0, AMIS_NONE,		0,  0, 0, AMIS_FLYALWAYS } };
			
// Standard altitude levels (int feet)
int HDelta[7] = { 0, 1, -1, 2, -2, 3, -3 };

#ifdef DEBUG
int notrim = 0;
#endif

static int sMissionProfile, sMissionAlt, sTargetAlt, sCruiseAlt, sCurrentAlt, sMissionMode, sRouteAction, sTargetDesc;
static int sCruiseSpeed, sMissionSpeed;

extern int LevelIncrement[ALT_LEVELS];
extern int IncrementMax[ALT_LEVELS];

int FindSafePath(WayPoint w1, WayPoint w2, Flight flight);
int CheckBestAltitude(GridIndex tx, GridIndex ty, Team who, int min, int max, int try_for, int type);
int ScoreThreatsOnWPLeg (WayPoint w1, WayPoint w2, Team who, int type);
WayPoint CheckSafePath (WayPoint w, WayPoint nw, Flight flight);
WayPoint AddSafeWaypoint(WayPoint w1, WayPoint w2, int type, int distance, Team who);
WayPoint AddDistanceWaypoint(WayPoint w1, WayPoint w2, int distance);
WayPoint EliminateExcessWaypoints (WayPoint w1, WayPoint w2, int who);
WayPoint FillAirPath (Path path, GridIndex *x, GridIndex *y, GridIndex nx, GridIndex ny, WayPoint w);

WayPoint SetupIngressPoints(WayPoint cw, Flight u, MissionRequestClass *mis);
WayPoint AddIngressPath(WayPoint cw, Flight u, MissionRequestClass *mis);
WayPoint AddAttackProfile(WayPoint cw, Flight u, MissionRequestClass *mis);
WayPoint AddLoiterProfile(WayPoint cw, Flight u, MissionRequestClass *mis);
WayPoint AddSearchProfile(WayPoint cw, Flight u, MissionRequestClass *mis);
WayPoint AddBypassProfile(WayPoint cw, Flight u, MissionRequestClass *mis);
WayPoint AddFlyByProfile(WayPoint cw, Flight u, MissionRequestClass *mis);
WayPoint AddLandProfile(WayPoint cw, Flight u, MissionRequestClass *mis);
WayPoint AddTargetProfile(WayPoint cw, Flight u, MissionRequestClass *mis);
WayPoint AddSweepProfile(WayPoint cw, Flight u, MissionRequestClass *mis);
WayPoint SetupEgressPoints(WayPoint cw, Flight u, MissionRequestClass *mis);
WayPoint AddEgressPath(WayPoint cw, Flight u, MissionRequestClass *mis);
WayPoint AddExitRoute (WayPoint cw, Flight u, MissionRequestClass *mis);

// ==========================================
// Global support functions
// ==========================================

void SetupAltitudes (Flight flight, MissionRequestClass *mis)
	{
	VehicleClassDataType*	vc;
	float		dtt;
	int			minAlt,maxAlt;
	GridIndex	x,y;

	// Collect some usefull data
	vc = GetVehicleClassData(flight->GetVehicleID(0));
	flight->GetLocation(&x,&y);
	dtt = Distance(x,y,mis->tx,mis->ty);
	sRouteAction = MissionData[mis->mission].routewp;
	sTargetDesc = MissionData[mis->mission].target_desc;

	// Set speeds
	sCruiseSpeed = flight->GetCruiseSpeed();
	sMissionSpeed = flight->GetCombatSpeed();
	if (mis->speed && MissionData[mis->mission].flags & AMIS_MATCHSPEED)
		sMissionSpeed = mis->speed;

	// Pick a profile (This could be done as a result of searching for an ingress path)
	if (MissionData[mis->mission].mission_profile == MPROF_STANDARD)
		{
		if (TeamInfo[flight->GetTeam()]->atm->averageCAMissions > MAX_ENEMY_CA_MISSIONS_FOR_HIGH_PROFILE)
			sMissionProfile = MPROF_LOW;
		else
			sMissionProfile = MPROF_HIGH;
		}
	else
		sMissionProfile = MissionData[mis->mission].mission_profile;

	// Choose an altitude at our target
	minAlt = MissionData[mis->mission].minalt*100;		// Minimum altitude at target
	maxAlt = MissionData[mis->mission].maxalt*100;		// Maximum altitude at target
	if (flight->IsHelicopter() && maxAlt > 0)
		minAlt = maxAlt = 100;
	else
		{
		if (maxAlt > vc->HighAlt*100)
			maxAlt = vc->HighAlt*100;
		if (minAlt < vc->LowAlt*100)
			minAlt = vc->LowAlt*100;
		}
	switch (mis->mission)
		{
		case AMIS_RECON:
			if (flight->GetSType() == STYPE_UNIT_RECON)
				{
				minAlt = maxAlt = vc->HighAlt*100;
				sMissionProfile = MPROF_HIGH;
				}
			break;
		default:
			break;
		}
	// Use mission alt by default
	if (sMissionProfile == MPROF_LOW)
		sTargetAlt = vc->LowAlt*100;
	else if (sMissionProfile == MPROF_HIGH)
		sTargetAlt = vc->HighAlt*100;
	else
		sTargetAlt = MissionData[mis->mission].missionalt*100;
	if (sTargetAlt < minAlt)
		sTargetAlt = minAlt;
	if (sTargetAlt > maxAlt)
		sTargetAlt = maxAlt;
	if (MissionData[mis->mission].target_profile != TPROF_FLYBY && MissionData[mis->mission].target_profile != TPROF_NONE)
		{
		// Find a target alt based on threat as well
		sTargetAlt = (sTargetAlt + MissionData[mis->mission].missionalt*100)/2;
		sTargetAlt = CheckBestAltitude(mis->tx, mis->ty, flight->GetTeam(), minAlt, maxAlt, sTargetAlt, TT_TOTAL);
		}

	// Pick a Mission Altitude
	if (dtt < MIN_DIST_FOR_INGRESS || DistanceToFront(mis->tx,mis->ty) < MIN_DIST_FOR_INGRESS/2)
		sMissionAlt = sTargetAlt;
	else if (sMissionProfile == MPROF_LOW)
		sMissionAlt = vc->LowAlt*100;
	else if (sMissionProfile == MPROF_HIGH)
		sMissionAlt = vc->HighAlt*100;
	else
		sMissionAlt = MissionData[mis->mission].missionalt*100;
	if (sMissionAlt < minAlt)
		sMissionAlt = minAlt;
	if (sMissionAlt > maxAlt)
		sMissionAlt = maxAlt;

	// Find our Cruise Altitude
	if (dtt < MIN_DIST_FOR_INGRESS)
		sCruiseAlt = sTargetAlt;
	else if (DistanceToFront(x,y) < MIN_DIST_FOR_CRUISE)
		sCruiseAlt = sMissionAlt;
	else
		sCruiseAlt = vc->CruiseAlt*100;						// Cruise altitude (over friendly territory)

	// Randomize our altitudes some
	int	alt_level;
	alt_level = GetAltitudeLevel(sMissionAlt);
	sMissionAlt += LevelIncrement[alt_level] * (mis->tx+mis->ty)%(alt_level+1);
	alt_level = GetAltitudeLevel(sTargetAlt);
	sTargetAlt += LevelIncrement[alt_level] * (mis->tx+mis->ty)%(alt_level+1);
	alt_level = GetAltitudeLevel(sCruiseAlt);
	sCruiseAlt += LevelIncrement[alt_level] * (mis->tx+mis->ty)%(alt_level+1);

	// 2001-12-31 ADDED BY S.G. Lets snap the altitude to increments of 500.
	if (sMissionAlt > 500)
		sMissionAlt = ((sMissionAlt + 250) / 500) * 500;
	if (sTargetAlt > 500)
		sTargetAlt = ((sTargetAlt + 250) / 500) * 500;
	if (sCruiseAlt > 500)
		sCruiseAlt = ((sCruiseAlt + 250) / 500) * 500;
	// END OF ADDED SECTION
	}

// This will set up the altitude we want all filler waypoints to be set to
void SetCurrentAltitude (void)
	{
	// Generally speaking, we want to use our last current altitude, unless:
	// a) We've just taken off
	if (sMissionMode == MMODE_TAKEOFF)
		sCurrentAlt = sMissionAlt; // sCruiseAlt;
	// b) We're at our assembly point
	if (sMissionMode == MMODE_AT_ASSEMBLY)
		sCurrentAlt = sMissionAlt;
	}

void CheckForClimb (WayPoint cw)
	{
	int			alt,altd;
	WayPoint	lw;

	// Check for max climb/decent
	alt = cw->GetWPAltitude();
	lw = cw->GetPrevWP();
	if (lw)
		{
		altd = alt - lw->GetWPAltitude();
		if (alt > 0 && abs(altd) > GRID_SIZE_FT)
			{
			GridIndex	x,y,lx,ly;
			int			maxdelta;
			// maxdelta is 1/2 distance we're travelling
			cw->GetWPLocation(&x,&y);
			lw->GetWPLocation(&lx,&ly);
			if (x == lx && y == ly)
				{
				// Waypoints co-located. need to add a new one
				WayPoint	nw;
				lx = x + SimToGrid((float)altd);
				ly = y;
				nw = new WayPointClass(lx, ly, lw->GetWPAltitude() + altd/2, 0, 0, 0, WP_NOTHING, 0);
				nw->SetWPRouteAction(lw->GetWPRouteAction());
				nw->SetWPSpeed(lw->GetWPSpeed());
				lw->InsertWP(nw);
				lw = nw;
				altd /= 2;
				}
			maxdelta = FloatToInt32((Distance(x,y,lx,ly)+1.0F) * g_fClimbRatio * GRID_SIZE_FT);
			if (maxdelta < abs(altd))
				{
				// This climb/decent is to steep. Set altitude to maximum delta.
				if (altd > 0)
					cw->SetWPAltitude( (int)((lw->GetWPAltitude() + maxdelta)/100) * 100);
				if (altd < 0)
					cw->SetWPAltitude( (int)((lw->GetWPAltitude() - maxdelta)/100) * 100);
				}
			}
		}
	}

// Update what segment of the mission we're in
// This should be called after each waypoint is set up.
void FinalizeWayPoint (WayPoint cw, int reset = FALSE)
	{
	static int	nextmode;
	int			alt = 0;

	if (reset)
		nextmode = -1;

	if (nextmode >= 0)
		{
		sMissionMode = nextmode;
		nextmode = -1;
		}

	// Check for Takeoff
	if (cw->GetWPAction() == WP_TAKEOFF)
		sMissionMode = MMODE_TAKEOFF;					
	// Check for Enroute
	else if (sMissionMode == MMODE_TAKEOFF)
		sMissionMode = MMODE_ENROUTE;							
	// Check if at assembly point (will be Ingress)
	if (cw->GetWPFlags() & WPF_ASSEMBLE && sMissionMode < MMODE_INGRESS)
		{
		sMissionMode = MMODE_AT_ASSEMBLY;
		nextmode = MMODE_INGRESS;	
		}
	// Check if at break point
	if (cw->GetWPFlags() & WPF_BREAKPOINT)
		sMissionMode = MMODE_AT_BREAKPOINT;
	// Check if in target area (Airlift/Aircav missions don't want takeoff waypoints to be in target area)
	if (sMissionMode > MMODE_TAKEOFF && (cw->GetWPFlags() & WPF_IP || cw->GetWPFlags() & WPF_TARGET || cw->GetWPFlags() & WPF_CP))
		sMissionMode = MMODE_IN_TARGET_AREA;			
	// Check if at turn point
	else if (cw->GetWPFlags() & WPF_TURNPOINT)
		sMissionMode = MMODE_AT_TURNPOINT;
	// Check for Egress
	else if (sMissionMode >= MMODE_IN_TARGET_AREA && sMissionMode < MMODE_EGRESS)
		sMissionMode = MMODE_EGRESS;			
	// Check for Post assembly (will be RTB)
	if ((cw->GetWPFlags() & WPF_ASSEMBLE) && sMissionMode >= MMODE_EGRESS && sMissionMode < MMODE_RETURN_TO_BASE)
		{
		sMissionMode = MMODE_AT_POSTASSEMBLY;	
		nextmode = MMODE_RETURN_TO_BASE;
		}
	// Check for Landing
	if (cw->GetWPAction() == WP_LAND)
		sMissionMode = MMODE_LANDING;

	// Check if in coordinated area "In Package" -> i.e: all package elements will fly this waypoint
	if (sMissionMode > MMODE_AT_ASSEMBLY && sMissionMode <= MMODE_AT_POSTASSEMBLY)
		{
		cw->SetWPFlag(WPF_IN_PACKAGE);
		cw->SetWPSpeed((float)sMissionSpeed);
		}
	else if (sMissionMode == MMODE_TAKEOFF)
		cw->SetWPSpeed(0.0F);
	else
		cw->SetWPSpeed((float)sCruiseSpeed);

	// Check if we should perform our route action or not (depends on target description)
// 2001-06-28 MODIFIED BY S.G. sMissionMode TAKES MANY VALUES! DON'T CHECK FOR EQUALITY BUT CHECKS FOR RANGE OF VALUES!
//	if ((sMissionMode == MMODE_IN_TARGET_AREA && sTargetDesc <= TDESC_TAO) || ((sMissionMode == MMODE_INGRESS || sMissionMode == MMODE_EGRESS) && sTargetDesc <= TDESC_ATA) || ((sMissionMode == MMODE_ENROUTE || sMissionMode == MMODE_RETURN_TO_BASE) && sTargetDesc <= TDESC_TTL))
	if ((sMissionMode == MMODE_IN_TARGET_AREA && sTargetDesc <= TDESC_TAO) || ((sMissionMode >= MMODE_INGRESS && sMissionMode <= MMODE_EGRESS) && sTargetDesc <= TDESC_ATA) || ((sMissionMode >= MMODE_ENROUTE && sMissionMode <= MMODE_RETURN_TO_BASE) && sTargetDesc <= TDESC_TTL))
		cw->SetWPRouteAction(sRouteAction);
	else
		cw->SetWPRouteAction(WP_NOTHING);

	// Choose altitude
	switch (sMissionMode)
		{
		case MMODE_TAKEOFF:
		case MMODE_LANDING:
			alt = 0;
			break;
		case MMODE_ENROUTE:	
		case MMODE_AT_ASSEMBLY:
		case MMODE_AT_POSTASSEMBLY:
		case MMODE_RETURN_TO_BASE:
			alt = sCruiseAlt;
			break;
		case MMODE_INGRESS:
		case MMODE_AT_BREAKPOINT:
		case MMODE_AT_TURNPOINT:
		case MMODE_EGRESS:
			alt = sMissionAlt;
			break;
		case MMODE_IN_TARGET_AREA:
			alt = sTargetAlt;
			break;
		default:
			ShiAssert(!"Shouldn't get here");
			break;
		}
	cw->SetWPAltitude(alt);
	sCurrentAlt = alt;

	// Check for max climb/decent
	CheckForClimb(cw);

	// I've noticed a problem where really high altitudes get generated.
	ShiAssert(cw->GetWPAltitude() < 100000);
//	ShiAssert(cw->GetWPAltitude() > 0 || cw->GetWPAction() == WP_LAND || cw->GetWPAction() == WP_TAKEOFF);

	// Set holdcurrent flag, if we need to
	if (sMissionMode == MMODE_RETURN_TO_BASE || (sMissionMode > MMODE_TAKEOFF && cw->GetWPAltitude() == 0))
		cw->SetWPFlags(cw->GetWPFlags() | WPF_HOLDCURRENT);
	}

// Special case of above function
// This should be called only if we know our mode isn't changing (after filler waypoints)
void FinalizeFillerWayPoint (WayPoint cw)
	{
	int			altd;
	WayPoint	lw;

	// Check if in coordinated area "In Package" -> i.e: all package elements will fly this waypoint
	if (sMissionMode >= MMODE_AT_ASSEMBLY && sMissionMode <= MMODE_AT_POSTASSEMBLY)
		{
		cw->SetWPFlag(WPF_IN_PACKAGE);
		cw->SetWPSpeed((float)sMissionSpeed);
		}
	else
		cw->SetWPSpeed((float)sCruiseSpeed);

	// Check if we should perform our route action or not (depends on target description)
// 2001-06-28 MODIFIED BY S.G. sMissionMode TAKES MANY VALUES! DON'T CHECK FOR EQUALITY BUT CHECKS FOR RANGE OF VALUES!
//	if ((sMissionMode == MMODE_IN_TARGET_AREA && sTargetDesc <= TDESC_TAO) || ((sMissionMode == MMODE_INGRESS || sMissionMode == MMODE_EGRESS) && sTargetDesc <= TDESC_ATA) || ((sMissionMode == MMODE_ENROUTE || sMissionMode == MMODE_RETURN_TO_BASE) && sTargetDesc <= TDESC_TTL))
	if ((sMissionMode == MMODE_IN_TARGET_AREA && sTargetDesc <= TDESC_TAO) || ((sMissionMode >= MMODE_INGRESS && sMissionMode <= MMODE_EGRESS) && sTargetDesc <= TDESC_ATA) || ((sMissionMode >= MMODE_ENROUTE && sMissionMode <= MMODE_RETURN_TO_BASE) && sTargetDesc <= TDESC_TTL))
		cw->SetWPRouteAction(sRouteAction);

	// Choose altitude
	cw->SetWPAltitude(sCurrentAlt);

	// Check for max climb/decent
	lw = cw->GetPrevWP();
	if (lw)
		{
		altd = sCurrentAlt - lw->GetWPAltitude();
		if (sCurrentAlt > 0 && abs(altd) > GRID_SIZE_FT)
			{
			// Just climb at max
			GridIndex	x,y,lx,ly;
			int			maxdelta;
			// maxdelta is 1/2 distance we're travelling
			cw->GetWPLocation(&x,&y);
			lw->GetWPLocation(&lx,&ly);
			maxdelta = FloatToInt32((Distance(x,y,lx,ly)+1.0F) * g_fClimbRatio * GRID_SIZE_FT);
			if (maxdelta < abs(altd))
				{
				// This climb/decent is to steep. Set altitude to maximum delta.
				if (altd > 0)
					cw->SetWPAltitude( (int)((lw->GetWPAltitude() + maxdelta)/100) * 100);
				if (altd < 0)
					cw->SetWPAltitude( (int)((lw->GetWPAltitude() - maxdelta)/100) * 100);
				}
			}
		}
	}

// This will build a path to a target and back and will build target area waypoints
// depending on the target profile.
int BuildPathToTarget (Flight u, MissionRequestClass *mis, VU_ID airbaseID)
	{
	WayPoint		nw,cw,sw;
	GridIndex		bx,by,ax,ay;
	Package			pack;
	int				exitroute = 0;
	CampEntity		airbase = FindEntity(airbaseID);

	if (!airbase)
		return 0;

	// Pointer to the package
	pack = (Package)u->GetUnitParent();
	SetupAltitudes(u, mis);

	// Takeoff waypoint (or filler wp for immediate missions
	airbase->GetLocation(&bx,&by);
	sw = cw = new WayPointClass(bx, by, 0, 0, 0, 0, WP_TAKEOFF, WPF_TAKEOFF);
	u->wp_list = cw;
	cw->SetWPTarget(airbase->Id());
	FinalizeWayPoint(cw,TRUE);
	SetCurrentAltitude();

	if (!(MissionData[mis->mission].flags & AMIS_TARGET_ONLY))
		{
		if (mis->mission == AMIS_AIRCAV)
			{
			// We've got to pick up our cargo first
			Unit		u = FindUnit(mis->requesterID);
			GridIndex	x,y;

			if (!u)
				return 0;
			u->GetLocation(&x,&y);
			nw = new WayPointClass(x, y, 0, 0, 0, MissionData[mis->mission].loitertime*CampaignMinutes, WP_PICKUP, WPF_LAND);
			nw->SetWPTarget(u->Id());
			FinalizeWayPoint(nw);
			cw->InsertWP(nw);
			cw = nw;
			// KCK EXPERIMENTAL: Try making taking off part of our 'PICKUP'..
//			nw = new WayPointClass(x, y, 0, 0, 0, 0, WP_TAKEOFF, WPF_TAKEOFF);
//			FinalizeWayPoint(nw);
//			cw->InsertWP(nw);
//			cw = nw;
			}

		// Ingress route
		pack->GetUnitAssemblyPoint(0,&ax,&ay);
		if (!ax || !ay || !pack->GetIngress())
			{
			// No assembly point currently- We need to find a path to the target, and determine
			// a good assembly point and break point from it.
			cw = SetupIngressPoints(cw,u,mis);
			if (!cw)
				return 0;
			}
		else
			{
			// Otherwise, find a path to the assembly point, and copy Ingress route
			cw = AddIngressPath(cw,u,mis);
			}
		}

	// Target area waypoints
	switch (MissionData[mis->mission].target_profile)
		{
		case TPROF_ATTACK:
			cw = AddAttackProfile(cw,u,mis);
			exitroute = 1;
			break;
		case TPROF_LOITER:
		case TPROF_SEARCH:
			cw = AddLoiterProfile(cw,u,mis);
			break;
		case TPROF_AVOID:
			cw = AddBypassProfile(cw,u,mis);
			break;
		case TPROF_FLYBY:
			cw = AddFlyByProfile(cw,u,mis);
			break;
		case TPROF_SWEEP:
			cw = AddSweepProfile(cw,u,mis);
			break;
		case TPROF_LAND:
			cw = AddLandProfile(cw,u,mis);
			break;
		case TPROF_HPATTACK:
		case TPROF_TARGET:
			cw = AddTargetProfile(cw,u,mis);
			exitroute = 1;
			break;
		case TPROF_NONE:
		default:
			// No target WP
			cw->SetWPTimes(mis->tot);
			exitroute = 1;
			break;
		}

	if (!(MissionData[mis->mission].flags & AMIS_TARGET_ONLY))
		{
		// Egress Route
		pack->GetUnitAssemblyPoint(1,&ax,&ay);
		if (!ax || !ay || !pack->GetEgress())
			{
			// No assembly point currently- We need to find a path to the target, and determine
			// a good assembly point and break point from it.
			if (exitroute)
				cw = AddExitRoute (cw,u,mis);
			cw = SetupEgressPoints(cw,u,mis);
			if (!cw)
				return 0;
			}
		else
			{
			// Otherwise, find a path to the assembly point, and copy Ingress route
			cw = AddEgressPath(cw,u,mis);
			}
		}

	// Route back to base
	nw = new WayPointClass(bx, by, 0, 0, 0, 0, WP_LAND, WPF_LAND | WPF_HOLDCURRENT);
	nw->SetWPTarget(airbase->Id());
	cw->InsertWP(nw);
	FinalizeWayPoint(nw);

	// Now let's try to eliminated unneeded waypoints for initial flight
	if (!u->GetUnitMissionID())
		EliminateExcessWaypoints (sw,nw,u->GetTeam());

	return 1;
	}

void BuildDivertPath (Flight flight, MissionRequestClass *mis)
	{
	WayPoint	cw,w,nw,tw;
	GridIndex	x,y;

	w = cw = flight->GetCurrentUnitWP();
	if (!cw)
		{
		MonoPrint("Problem - airborne flight with no waypoints!n");
		return;
		}

	SetupAltitudes(flight, mis);

	tw = AddTargetProfile(NULL,flight,mis);
	tw->SetWPFlags(w->GetWPFlags() | WPF_DIVERT | WPF_TARGET);
	tw->SetWPAltitude(MissionData[mis->mission].missionalt);

	if (w->GetWPAction() == WP_TAKEOFF || !w->GetPrevWP())
		{
		// This thing hasn't taken off yet, so plan a real route
		w->InsertWP(tw);
		SetWPTimes (w, TheCampaign.CurrentTime + CampaignMinutes, sMissionSpeed, 0);
		}
	else
		{
		// Otherwise only set up the override waypoint
		if (MissionData[mis->mission].flags & AMIS_ASSIGNED_TAR)
			{
			// Just assign us a target and an override waypoint
			flight->SetAssignedTarget(mis->targetID);
			FinalizeWayPoint(tw,TRUE);
			if (mis->flags & AMIS_HELP_REQUEST)
				flight->SetOverrideWP(tw, true);
			else
				flight->SetOverrideWP(tw);
			delete tw;
			tw = flight->GetOverrideWP();
			}
		else
			{
			// Otherwise, insert it into our list
			nw = cw->GetPrevWP();
			nw->UnlinkNextWP();
			nw->InsertWP(tw);
			tw->InsertWP(cw);
			tw->SetWPAltitude(MissionData[mis->mission].missionalt);
			}
		flight->GetLocation(&x,&y);
		SetWPTimes (tw, x, y, flight->GetCombatSpeed(), 0);
		}
	}

// =========================================
// Waypoint manipulation support funcations
// =========================================

// Takes a wp path from airbase (cw) to target and adds in an Assembly Point and a Break
// Point, saving these values to the parent package, removes the target WP, and returns the
// last wp in the list (break point).
WayPoint SetupIngressPoints(WayPoint cw, Flight u, MissionRequestClass *mis)
	{
	Unit			pack;
	WayPoint		nw,tw,dw,aw;
	GridIndex		iax,iay,bpx,bpy;

	pack = u->GetUnitParent();

	// Find the path to the target
	tw = new WayPointClass(mis->tx, mis->ty, 10000, 0, 0, 0, WP_NOTHING, WPF_TARGET);
	cw->UnlinkNextWP ();
	cw->InsertWP(tw);
	if (!CheckSafePath(cw,tw,u))
		return 0;

	// Find a safe location for an assembly point and add it to the list
	aw = AddSafeWaypoint(cw,tw,0,MIN_AP_DISTANCE,u->GetTeam());
	aw->SetWPAction(WP_ASSEMBLE);
	aw->GetWPLocation(&iax,&iay);
	FinalizeWayPoint(aw);
	pack->SetUnitAssemblyPoint(0,iax,iay);

	if (!(MissionData[mis->mission].flags & AMIS_NO_BREAKPT))
		{
		// Find a good breakpoint
		nw = AddDistanceWaypoint(aw,tw,BREAKPOINT_DISTANCE*2);
		nw->SetWPFlag(WPF_BREAKPOINT);
		nw->GetWPLocation(&bpx,&bpy);
		FinalizeWayPoint(nw);
		pack->SetUnitAssemblyPoint(2,bpx,bpy);
		}
	else
		nw = aw;

	// Remove target area waypoints
	dw = nw->GetNextWP();
	while (dw)
		{
		tw = dw->GetNextWP();
		dw->DeleteWP ();
		dw = tw;
		}

	return nw;
	}

// Copies the package's ingress path into this unit's wp path starting at cw
WayPoint AddIngressPath(WayPoint cw, Flight u, MissionRequestClass *mis)
	{
	WayPoint		nw;
	Unit			pack;

	pack = u->GetUnitParent();
	nw = CloneWPList(((Package)pack)->GetIngress());
	cw->InsertWP(nw);
	if (MissionData[mis->mission].flags & AMIS_DONT_COORD)
		{
		// Reset ingress times to 0
		WayPoint temp = nw;
		while (temp)
			{
			temp->SetWPTimes(0);
			temp = temp->GetNextWP();
			}
		}
	while (cw && cw->GetNextWP())
		{
		FinalizeWayPoint(cw);
		cw = cw->GetNextWP();
		}
	FinalizeWayPoint(cw);
	return cw;
	}
	
// Adds an IP, Target WP, and a Turn point (safe turning location, past target)
WayPoint AddAttackProfile(WayPoint cw, Flight u, MissionRequestClass *mis)
	{
	GridIndex		ipx,ipy,tpx,tpy,bx,by;
	WayPoint		tw,tpw,nw;
	Unit			pack;

	pack = u->GetUnitParent();
	u->GetLocation(&bx,&by);

	// Add the target WP
	tw = new WayPointClass(mis->tx, mis->ty, 0, 0, 0, 0, MissionData[mis->mission].targetwp, WPF_TARGET);
	cw->InsertWP(tw);
	tw->SetWPTarget(mis->targetID);
	tw->SetWPTargetBuilding((uchar)mis->target_num);
	
	// Find and add the IP
	nw = AddDistanceWaypoint(cw,tw,BREAKPOINT_DISTANCE);
	nw->SetWPFlag(WPF_IP);
	FinalizeWayPoint(nw);
	FinalizeWayPoint(tw);

	// Find and add the FIRST turn point, if we don't have one
	pack->GetUnitAssemblyPoint(3,&tpx,&tpy);
	if (!tpx || !tpy)
		{
		int			i,s,bs=9999,fh,h,d;
		GridIndex	x,y;
		// Look around for a safe spot
		nw->GetWPLocation(&ipx,&ipy);
		fh = (int)DirectionTo(ipx,ipy,mis->tx,mis->ty);
		tpw = new WayPointClass(0, 0, 0, 0, 0, 0, WP_NOTHING, WPF_TURNPOINT);
		for (i=0; i<5; i++)
			{
			h = (fh + HDelta[i] + 8)%8;
			if (dx[h] && dy[h])
				d = FloatToInt32(0.707F * BREAKPOINT_DISTANCE);
			else
				d = BREAKPOINT_DISTANCE;
			x = mis->tx + dx[h] * d;
			y = mis->ty + dy[h] * d;
			s = ScoreThreatFast(x,y,GetAltitudeLevel(sTargetAlt),u->GetTeam()) - i;
			s += ScoreThreatFast(x+dx[h]*d,y+dy[h]*d,GetAltitudeLevel(sTargetAlt),u->GetTeam());
			if (s < bs || (s==bs && DistSqu(x,y,bx,by) < DistSqu(tpx,tpy,bx,by)))
				{
				tpx = x;
				tpy = y;
				bs = s;
				}
			}
		pack->SetUnitAssemblyPoint(3,tpx,tpy);
		tpw->SetWPLocation(tpx,tpy);
		FinalizeWayPoint(tpw);
		tw->InsertWP(tpw);
		tw = tpw;
		}

	return tw;
	}

// Adds two loiter waypoints, which repeat until out of fuel
WayPoint AddLoiterProfile(WayPoint cw, Flight u, MissionRequestClass *mis)
	{
	GridIndex		lx,ly;
	WayPoint		tw,lw;
	float			r;

	// The Target Location
	tw = new WayPointClass(mis->tx, mis->ty, 0, 0, 0, 0, MissionData[mis->mission].targetwp, WPF_TARGET | WPF_CP);
	tw->SetWPTarget(mis->targetID);
	tw->SetWPTargetBuilding((uchar)mis->target_num);
	FinalizeWayPoint(tw);
	cw->InsertWP(tw);
	cw = tw;

	// The Loiter Location (loop back to previous wp)
	r = DirectionTowardFriendly(mis->tx,mis->ty,u->GetTeam());		// Direction away from front, essentially
	// Special case for FAC missions - direction towards enemy
	if (mis->mission == AMIS_FAC)
		r += PI;
	lx = mis->tx + (GridIndex)(LOITER_DIST*sin(r));
	ly = mis->ty + (GridIndex)(LOITER_DIST*cos(r));
	lw = new WayPointClass(lx, ly, 0, 0, 0, MissionData[mis->mission].loitertime*CampaignMinutes, MissionData[mis->mission].targetwp, WPF_TARGET | WPF_CP | WPF_REPEAT);
	FinalizeWayPoint(lw);
	cw->InsertWP(lw);
	cw = lw;

	return cw;
	}

// Finds a safe path between the break point and the turn point
WayPoint AddBypassProfile(WayPoint cw, Flight u, MissionRequestClass *mis)
	{
	return cw;
	}

// Flys from break point to target and on to regroup point, staying at safest altitude
WayPoint AddFlyByProfile(WayPoint cw, Flight u, MissionRequestClass *mis)
	{
//	GridIndex		tpx,tpy;
	WayPoint		tw;
	Unit			pack;

	pack = u->GetUnitParent();

	// Target WP
	tw = new WayPointClass(mis->tx, mis->ty, 0, 0, 0, 0, MissionData[mis->mission].targetwp, WPF_TARGET);
	FinalizeWayPoint(tw);
	cw->InsertWP(tw);
	cw = tw;

	return cw;
	}

// Adds three waypoints forming a half circle around the target area
WayPoint AddSweepProfile(WayPoint cw, Flight u, MissionRequestClass *mis)
	{
	GridIndex	sx[3],sy[3],x,y;
	int			i,s,ls,score,bs,fh,ch,h,d;
	int			ad[3] = { -2, 0, 2 };
	WayPoint	sw,fw;

	// Sweep around mission destination point
	u->GetLocation (&x,&y);
	fh = (int)DirectionTo(x,y,mis->tx,mis->ty);
	fw = cw;

	// Look around for safe spots (we want 3)
	for (s=0; s<3; s++)
		{
		bs = 9999;
		sw = new WayPointClass(0, 0, 0, 0, 0, 0, MissionData[mis->mission].targetwp, WPF_TARGET);
		sw->SetWPTarget(mis->targetID);
		sw->SetWPTargetBuilding((uchar)mis->target_num);
		ch = (fh + ad[s] + 8)%8; 
		for (i=0; i<3; i++)
			{
			h = (ch + HDelta[i] + 8)%8;
			if (dx[h] && dy[h])
				d = FloatToInt32((.707F * SWEEP_DISTANCE));
			else
				d = SWEEP_DISTANCE;
			x = mis->tx + dx[h] * d;
			y = mis->ty + dy[h] * d;
			score = ScoreThreatFast(x,y,GetAltitudeLevel(sTargetAlt),u->GetTeam()) + i;
			// Check if the previous guy grabbed this point
			for (ls=0; ls<s; ls++)
				{
				if (sx[ls] == x && sy[ls] == y)
					score = 100;
				}
			if (score < bs)
				{
				sx[s] = x;
				sy[s] = y;
				bs = score;
				}
			}
		sw->SetWPLocation(sx[s],sy[s]);
		FinalizeWayPoint(sw);
		cw->InsertWP(sw);
		cw = sw;
		}
	return cw;
	}

// Adds a land && takeoff WP at target
WayPoint AddLandProfile(WayPoint cw, Flight u, MissionRequestClass *mis)
	{
	WayPoint		tw;

	// Mark both takeoff and land flags, since we're not staying here.
	tw = new WayPointClass(mis->tx, mis->ty, 0, 0, 0, MissionData[mis->mission].loitertime*CampaignMinutes, MissionData[mis->mission].targetwp, WPF_TARGET | WPF_LAND | WPF_TAKEOFF);
	tw->SetWPTarget(mis->targetID);
	tw->SetWPTargetBuilding((uchar)mis->target_num);
	FinalizeWayPoint(tw);
	if (cw)
		cw->InsertWP(tw);
	cw = tw;
	// KCK: Experimental - Fold the takeoff bit into previous waypoint.
//	tw = new WayPointClass(mis->tx, mis->ty, 0, 0, 0, 0, WP_TAKEOFF, WPF_TARGET | WPF_TAKEOFF);
//	FinalizeWayPoint(tw);
//	cw->InsertWP(tw);
	return tw;
	}

// Adds a target wp only
WayPoint AddTargetProfile(WayPoint cw, Flight u, MissionRequestClass *mis)
	{
	WayPoint		tw;

	tw = new WayPointClass(mis->tx, mis->ty, 0, 0, 0, 0, MissionData[mis->mission].targetwp, WPF_TARGET);
	tw->SetWPTarget(mis->targetID);
	tw->SetWPTargetBuilding((uchar)mis->target_num);
	FinalizeWayPoint(tw);
	if (cw)
		cw->InsertWP(tw);
	return tw;
	}

// Finds a path from waypoint cw to home base, and finds and saves an egress assembly point
WayPoint SetupEgressPoints(WayPoint cw, Flight u, MissionRequestClass *mis)
	{
	WayPoint		nw,bw,tw,dw;
	GridIndex		eax,eay,bx,by;
	Unit			pack;
	
	pack = u->GetUnitParent();

	// Finalize route from exit WP to base
	u->GetLocation(&bx,&by);
	bw = new WayPointClass(bx, by, 0, 0, 0, 0, WP_LAND, WPF_LAND | WPF_HOLDCURRENT);
	cw->InsertWP(bw);
	if (!CheckSafePath(cw,bw,u))
		return 0;

	// Find a safe location for a post assembly point and add it to the list
	nw = AddSafeWaypoint(bw,cw,1,MIN_AP_DISTANCE,u->GetTeam());
	nw->GetWPLocation(&eax,&eay);
	pack->SetUnitAssemblyPoint(1,eax,eay);
	nw->SetWPAction(WP_POSTASSEMBLE);
	nw->SetWPFlag(WPF_HOLDCURRENT);
	nw->SetWPRouteAction(WP_NOTHING);
	FinalizeWayPoint(nw);
	dw = nw->GetNextWP();

	// Remove remaining waypoint from assembly point to base - We'll add them in later with
	// another scheme.
	while (dw)
		{
		tw = dw->GetNextWP();
		dw->DeleteWP ();
		dw = tw;
		}

	return nw;
	}

// Copies the package's egress path into this unit's wp path
WayPoint AddEgressPath(WayPoint cw, Flight u, MissionRequestClass *mis)
	{
	WayPoint			nw;
	Package				pack;

	pack = (Package)u->GetUnitParent();
	nw = CloneWPList(pack->GetEgress());
	if (MissionData[mis->mission].flags & AMIS_DONT_COORD)
		{
		// Don't tie egress times to package
		WayPoint temp = nw;
		while (temp)
			{
			temp->SetWPTimes(0);
			temp = temp->GetNextWP();
			}
		}
	cw->InsertWP(nw);
	cw = nw;
	while (cw && cw->GetNextWP())
		{
		FinalizeWayPoint(cw);
		cw = cw->GetNextWP();
		}
	FinalizeWayPoint(cw);
	return cw;
	}

// Places a waypoint at a good turn location nearby target. Techincally part of egress route, when desired
WayPoint AddExitRoute (WayPoint cw, Flight u, MissionRequestClass *mis)
	{
	WayPoint		nw,bw;
	GridIndex		eax,eay,bx,by,tx,ty,x,y;
	Unit			pack;
	int				i,s,bs=99999,fh,h,d;

	pack = u->GetUnitParent();

	// Find a good egress direction (from target or turn point)
	cw->GetWPLocation(&tx,&ty);						// Last way point (Turn point or target)
	fh = (int)DirectionTo(mis->tx,mis->ty,tx,ty);
	if (fh == Here)
		{
		nw = cw->GetPrevWP();	// Pre-target WP
		nw->GetWPLocation(&tx,&ty);
		fh = (int)(DirectionTo(mis->tx,mis->ty,tx,ty) + 4)%8;
		}
	// Add the new WP to path (note: it's location hasn't been determined yet)
	nw = new WayPointClass(0, 0, 0, 0, 0, 0, WP_NOTHING, 0);
	FinalizeWayPoint(nw);
	cw->InsertWP(nw);

	// Now add the home bases' location, for checking best exit route
	u->GetLocation(&bx,&by);
	bw = new WayPointClass(bx, by, 0, 0, 0, 0, WP_LAND, WPF_LAND | WPF_HOLDCURRENT);
	bw->SetWPFlags(WPF_LAND);
	nw->InsertWP(bw);

	eax = bx;
	eay = by;

	// Now determine best exit direction
	for (i=3; i<7; i++)
		{
		h = (fh + HDelta[i] + 8)%8;
		if (dx[h] && dy[h])
			d = FloatToInt32(0.707F * BREAKPOINT_DISTANCE);
		else
			d = BREAKPOINT_DISTANCE;
		x = tx + dx[h] * d;
		y = ty + dy[h] * d;
		s = ScoreThreatFast(x,y,GetAltitudeLevel(sMissionAlt),u->GetTeam()) + i;
		s += ScoreThreatFast(x+dx[h]*d,y+dy[h]*d,GetAltitudeLevel(sMissionAlt),u->GetTeam());
		if (s < bs || (s==bs && DistSqu(x,y,bx,by) < DistSqu(eax,eay,bx,by)))
			{
			eax = x;
			eay = y;
			bs = s;
			}
		}
	//if this asserts ScoreThreatFast probably has an error
	ShiAssert(bs < 99999);
	nw->SetWPLocation(eax,eay);
	cw = nw;

	// Now delete the home base waypoint (we'll add this when we do our egress
	bw->DeleteWP ();
	return cw;
	}

// Add WP for Tanker, Abort strip, and Secondary Target
void AddInformationWPs (Flight flight, MissionRequestClass *mis)
	{
	WayPoint		w,lw;
	int				d;
	GridIndex		x,y;
	Objective		o;
	CampaignTime	time;

	// KCK NOTE: This assumes the flight is still owned by the ATM's machine
	// If not, abort..
	if (!TeamInfo[flight->GetTeam()]->atm->IsLocal())
		return;

	x = y = 0;
	lw = flight->wp_list;
	while (lw && lw->GetNextWP())
		{
		if (!x && lw->GetWPFlags() & WPF_ASSEMBLE)
			{
			lw->GetWPLocation(&x,&y);
			time = lw->GetWPArrivalTime();
			}
		lw = lw->GetNextWP();
		}

	// Find Tanker (Use assembly points, if possible, otherwise target)
	if (!x)
		{
		x = mis->tx;
		y = mis->ty;
		time = mis->tot;
		}
	if (flight->GetUnitMission() != AMIS_TANKER)
		{
 		d = TeamInfo[flight->GetTeam()]->atm->FindNearestActiveTanker(&x,&y,&time);
		if (d < MAXIMUM_TANKER_DISTANCE*4)
			{
			w = new WayPointClass(x, y, 20000, 0, 0, 0, WP_REFUEL, WPF_REFUEL_INFORMATION);	// M.N. added REFUEL_INFORMATION flag
//			w->SetWPTarget(tanker);
			lw->InsertWP(w);
			lw = w;
			}
		}

	// Find Secondary Target
	// KCK NOTE: Do Later

	// Find Abort Strip
	o = FindAlternateStrip ((Flight)flight);
	if (o)
		{
		o->GetLocation(&x,&y);
		w = new WayPointClass(x, y, 0, 0, 0, 0, WP_LAND, WPF_ALTERNATE | WPF_LAND | WPF_HOLDCURRENT);
		w->SetWPTarget(o->Id());
		lw->InsertWP(w);
		lw = w;
		}
	}

void ClearDivertWayPoints (Flight flight)
	{
	WayPoint		w,nw;

	w = flight->GetFirstUnitWP();
	while (w)
		{
		if (w->GetWPFlags() & WPF_DIVERT)
			{
			nw = w;
			w = w->GetPrevWP();
			nw->DeleteWP();
			}
		w = w->GetNextWP();
		}
	}

// 2001-10-12 ADDED BY M.N. -> Adds a refuel waypoint if a tanker is needed

int AddTankerWayPoint (Flight u, int refuel)
	{
	WayPoint		w = NULL,lw = NULL,fw = NULL,sw = NULL,bw = NULL;
	float			dist,wdist;
	Int32			d,bd;
	int				fuel, fuelNeeded;
	GridIndex		x,y,ix,iy,wx,wy;
//	Objective		o;		Use CampMap team locator instead of objective
	Package			pack;
	MissionRequestClass *mis;	
	CampaignTime	time,totime,landtime,ingresstime;
	int				wpinserted = 0;
	long			length;

	// This assumes the flight is still owned by the ATM's machine
	// If not, return..
	if (!TeamInfo[u->GetTeam()]->atm->IsLocal())
		return 1;
	
	if (refuel < g_nNoWPRefuelNeeded)
		return 1;

	// Okay, we will now - hopefully at least added one regular tanker waypoint,
	// remove a possible tanker information waypoint, which is not 
	// needed anymore, and get takeoff & land waypoint for excess elimination

	lw = u->wp_list;
	while (lw)
	{
		bw = lw;
		// only delete tanker information WP, not regular refueling WP
		if (bw->GetWPAction() == WP_REFUEL && (bw->GetWPFlags() & WPF_REFUEL_INFORMATION)) 
			bw->DeleteWP();
		if (lw->GetWPAction() == WP_TAKEOFF)
		{
			totime = lw->GetWPDepartureTime();
			sw = lw;
		}
		if (lw->GetWPAction() == WP_LAND)
		{
			landtime = lw->GetWPArrivalTime();
			fw = lw;
		}
		lw = lw->GetNextWP();
	}

	x = y = ix = iy = wx = wy = 0;

	pack = u->GetUnitPackage();
	mis = pack->GetMissionRequest();
	SetupAltitudes (u,mis);

	fuel = u->CalculateFuelAvailable(255); // this gives us the loaded fuel (+ extra tanks)

	// if needed fuel (==refuel) is less than the flight 
	// unit's fuel * 2/3, only add a refuel waypoint at 
	// egress, if more, add another one at ingress.

	lw = u->wp_list;
	if (refuel > (2*fuel/3) && g_bAddIngressWP)	// Add a Tanker waypoint at ingress
	{
		// Find the ingress waypoint closest to an active tanker
		bd = 9990;
		bw = NULL;
		while (lw && lw->GetNextWP())
		{
			lw->GetWPLocation(&x,&y);
			wx = x;
			wy = y;
			time = lw->GetWPArrivalTime();
			d = TeamInfo[u->GetTeam()]->atm->FindNearestActiveTanker(&x,&y,&time);
			if (d < bd)
			{
				bd = d;
				bw = lw;
				ix = x;
				iy = y;
			}
//			o = FindNearestObjective(wx,wy,NULL); // objective near the waypoint => marker for territory
			if (::GetOwner(TheCampaign.CampMapData, wx, wy) != u->GetTeam()) // abort search if we got into enemy territory
			{
				break;
			}
			lw = lw->GetNextWP();
		}

		if (ix != 0 && iy != 0)	// -> We have found a tanker near a waypoint
		{
			dist = DistanceToFront(ix,iy); // Tankers distance to FLOT
			w = new WayPointClass(ix, iy, 20000, 0, 0, 0, WP_REFUEL, 0);
//			o = FindNearestObjective(wx,wy,NULL);
			// We can have the case that a waypoint on the other side of the FLOT
			// is closer to an active tanker at its mistot time than a waypoint on friendly side.
			// In this case, go one waypoint back
			if (::GetOwner(TheCampaign.CampMapData, wx, wy) != u->GetTeam() && bw->GetPrevWP())
				bw = bw->GetPrevWP();		// Is wx,wy right here ?????????????????
			
			bw->GetWPLocation(&x,&y);
			wdist = DistanceToFront(x,y);
			if (wdist < dist)
				bw->InsertWP(w);
			else
			{
				bw = bw->GetNextWP();
				// Do the friendly territory check here again
				bw->GetWPLocation(&x,&y);
//				o = FindNearestObjective(x,y,NULL);
				if (::GetOwner(TheCampaign.CampMapData, x, y) != u->GetTeam() && bw->GetPrevWP())
					bw = bw->GetPrevWP();
				bw->InsertWP(w);
			}
			FinalizeWayPoint(w);
			wpinserted++;
			SetWPTimesTanker(u,mis,true, landtime);
			// Check if we can reach this first tanker waypoint with our loaded fuel,
			// otherwise kill this unit
			ingresstime = w->GetWPDepartureTime();
			length = w->GetWPArrivalTime() - totime;
			fuelNeeded = ((int)(length/CampaignMinutes) * u->GetClassData()->Rate);
			if (fuelNeeded > fuel*1.5) // way to tanker, 50% tolerance
			{
#ifdef DEBUG
				MonoPrint ("Flight %d can't reach ingress tanker",u->GetCampID());
#endif
				return 0;
			}
		}
	}

	ix = iy = 0;
	lw = u->wp_list;
	// Get the landing waypoint, and backwards browse the wp list for the best egress wp
	while (lw && lw->GetNextWP())
	{
		if (lw->GetWPAction() == WP_LAND)
		{
			break;
		}
		lw = lw->GetNextWP();
	}

	// Find the egress waypoint closest to an active tanker

	bd = 9990;
	bw = NULL;
	while (lw && lw->GetPrevWP())
	{
		lw->GetWPLocation(&x,&y);
		wx = x;
		wy = y;
		time = lw->GetWPArrivalTime();
		d = TeamInfo[u->GetTeam()]->atm->FindNearestActiveTanker(&x,&y,&time);
		if (d < bd)
		{
			bd = d;
			bw = lw;
			ix = x;
			iy = y;
		}
//		o = FindNearestObjective(wx,wy,NULL);
		if (::GetOwner(TheCampaign.CampMapData, wx, wy) != u->GetTeam()) // abort search if we got into enemy territory
		{
			//bw = lw;
			break;
		}
		lw = lw->GetPrevWP();
	}

	if (ix != 0 && iy != 0)	// -> We have found a tanker, otherwise ix==iy==0;
	{
		dist = DistanceToFront(ix,iy); // Tankers distance to FLOT
		w = new WayPointClass(ix, iy, 20000, 0, 0, 0, WP_REFUEL, 0);
		bw->GetWPLocation(&x,&y);
		wdist = DistanceToFront(x,y);
		if (wdist < dist)
			bw->InsertWP(w);
		else
		{
			bw = bw->GetPrevWP();
			bw->InsertWP(w);
		}
		FinalizeWayPoint(w);
		wpinserted++;
		length = SetWPTimesTanker(u,mis,false, totime);
		if (wpinserted == 2)	// we already have an ingress waypoint
		{
			length = w->GetWPArrivalTime() - ingresstime;
			fuelNeeded = ((int)(length/CampaignMinutes) * u->GetClassData()->Rate);
			if (fuelNeeded > fuel*1.5) // way ingresstanker -> egresstanker, 50% tolerance
				return 0;
		}
		else		// only an egress waypoint
		{
			length = w->GetWPArrivalTime() - totime;
			fuelNeeded = ((int)(length/CampaignMinutes) * u->GetClassData()->Rate);
			if (fuelNeeded > fuel*1.5) // way takeoff -> egresstanker, 50% tolerance
			{
#ifdef DEBUG
		ShiWarning ("Takeoff to egresstanker can't be reached - needs adjusting the 2D fuel rates");
#endif
				return 0;
			}
		}
	}
	if (!wpinserted)
		return 0;
	EliminateExcessWaypoints(sw,fw,u->GetTeam()); // remove unneeded wpts from Takeoff to Land
	return 1;
}




int WayPointErrorCode (WayPointClass *wp, Flight flt);

// Adjusts times for a waypoint list, and returns mission length
long SetWPTimes (Flight u, MissionRequestClass *mis)
	{
	WayPoint		w,fw,tw=NULL;
	CampaignTime	mission_time,to,land,time;
	long			offset;
	float			minSpeed,maxSpeed,speed,dist;
	GridIndex		x,y,nx,ny;
	Package			pack;

	fw = w = u->GetFirstUnitWP();
	if (!w)
		return 0;

	pack = (Package) u->GetUnitParent();
	if (!pack)
		return 0;
//TJL 11/22/03 More division by 2 removal. Errors are now aircraft specific.
	// Removed cruise/max distinctions.  All speeds are based on MoveSpeed (in KM).
	//minSpeed = u->GetCruiseSpeed()/2.0F;
	//maxSpeed = (float)u->GetMaxSpeed();
	minSpeed = (float)u->GetCruiseSpeed() * 0.7F;
	maxSpeed = (float)u->GetMaxSpeed() * 1.3F;



	offset = MissionData[u->GetUnitMission()].separation*CampaignSeconds;

	// Fix the offset for any fixed wps (time should == 0 for any wps needing setting)
	if (mis->tot_type == TOT_TAKEOFF)
		{
		// Fixed Takeoff time
		tw = fw;
		tw->SetWPTimes(mis->tot);
		}
	else if (mis->tot_type == TOT_ENROUTE)
		{
		// Fixed enroute time
		tw = fw;
		tw->SetWPTimes(mis->tot);
		}
	else if (mis->tot_type == TOT_INGRESS)
		{
		// Fixed ingrss time
		tw = fw;
		while (tw && !(tw->GetWPFlags() & WPF_IN_PACKAGE))
			tw = tw->GetNextWP();
		if (tw)
			tw->SetWPTimes(mis->tot);
		}
	while (w)
		{
		if (w->GetWPArrivalTime())
			w->SetWPTimes(w->GetWPArrivalTime() + offset);
		if (w->GetWPFlags() & WPF_TARGET && !tw)
			{
			tw = w;
			w->SetWPTimes(mis->tot);
			}
		if (w->GetWPFlags() & WPF_TURNPOINT && !tw)
			tw = w;
		// Lock time in some cases
		w->UnSetWPFlag(WPF_SPEED_LOCKED);
		if (w->GetWPAction() == WP_TAKEOFF || (w->GetWPFlags() & WPF_TARGET) || (w->GetWPFlags() & WPF_ASSEMBLE))
			w->SetWPFlag(WPF_TIME_LOCKED);
		w = w->GetNextWP();
		}

	// Now let's try and set any times we're allowed to (target to landing first)
	if (!tw)
		tw = fw;
	mission_time = tw->GetWPDepartureTime();
	tw->GetWPLocation(&x,&y);
	ShiAssert (mission_time > 0.0F);
	w = tw->GetNextWP();
	while (w)
		{
		w->GetWPLocation(&nx,&ny);
		dist = Distance(x,y,nx,ny);
		if (w->GetWPArrivalTime() > 0)			// Preset time - Adjust Speed to match
			{
			ShiAssert(w->GetWPArrivalTime() > mission_time);
			time = w->GetWPArrivalTime() - mission_time;
			speed = (dist * CampaignHours) / time;
			if (speed < minSpeed)
				speed = minSpeed;
			if (speed > maxSpeed)
				speed = maxSpeed;
			w->SetWPSpeed(speed);
			}
		else
			speed = w->GetWPSpeed();
		mission_time += TimeToArrive(dist,speed);
		w->SetWPTimes(mission_time);
		// Set package turn point time, as a secondary syncronization point
		if (w->GetWPFlags() == WPF_TURNPOINT && ((Package)pack)->GetTPTime() < 1.0F)
			((Package)pack)->SetTPTime (mission_time);
		mission_time += w->GetWPStationTime();
#ifdef DEBUG
//		if (u->GetUnitMission() != AMIS_ALERT && u->GetUnitMission() != AMIS_RECONPATROL)
//			ShiAssert(!WayPointErrorCode(w,u));
#endif
		w = w->GetNextWP();
		x = nx; y = ny;
		}
	land = mission_time;		// This is landing time

	// Backwards, from target to takeoff
	mission_time = tw->GetWPArrivalTime();
	tw->GetWPLocation(&x,&y);
	w = tw->GetPrevWP();
	unsigned long prevmission_time;
	while (w)
		{
		w->GetWPLocation(&nx,&ny);
		dist = Distance(x,y,nx,ny);
		if (w->GetWPArrivalTime() > 0)			// Preset time
			{
//			ShiAssert(mission_time > w->GetWPDepartureTime());
			time = mission_time - w->GetWPDepartureTime();
			speed = (dist * CampaignHours) / time;
			if (speed < minSpeed)
				speed = minSpeed;
			if (speed > maxSpeed)
				speed = maxSpeed;
			w->GetNextWP()->SetWPSpeed(speed);
			mission_time = w->GetWPArrivalTime();
			}
		else
			{
			speed = w->GetNextWP()->GetWPSpeed();
			if (w->GetWPAction() == WP_TAKEOFF)
				prevmission_time = mission_time;
			mission_time -= TimeToArrive(dist,speed);
			mission_time -= w->GetWPStationTime();
			}
// 2002-03-21 MN give a bit more time at takeoff
/*		if (w->GetWPAction() == WP_TAKEOFF)
		{
			mission_time -= 15000; // we need a bit more time to take off...
			w->SetWPTimes(mission_time);
			time = prevmission_time - w->GetWPDepartureTime();
			speed = (dist * CampaignHours) / time;
			if (speed < minSpeed)
				speed = minSpeed;
			if (speed > maxSpeed)
				speed = maxSpeed;
			w->GetNextWP()->SetWPSpeed(speed);
		}
		else*/
			w->SetWPTimes(mission_time);
		w = w->GetPrevWP();
		x = nx; y = ny;
		}
	to = mission_time;			// This is takeoff time

	// Calculate length and check for impossible takeoff times
// 2001-10-31 MODIFIED by M.N. Added new flag check for TE Missions. 
// In TE Planner, to < CurrentTime is always the case, thus we get a wrong mission length
	if (mis->flags & REQF_TE_MISSION)
		return land - to;
	if (to < TheCampaign.CurrentTime)
		return (long)to - (long)TheCampaign.CurrentTime;
	if (land > to)
		return land - to;
	return 0;
	}

long SetWPTimesTanker (Flight u, MissionRequestClass *mis, bool type, CampaignTime time)
// type == true: Ingress refuel waypoint - adapt 
// type == false: Egress refuel waypoint
	{
	WayPoint		w,fw,tw=NULL;
	CampaignTime	mission_time;
	float			minSpeed,maxSpeed,speed,dist;
	GridIndex		x,y,nx,ny;
	Package			pack;

	fw = w = u->GetFirstUnitWP();
	if (!w)
		return 0;

	pack = (Package) u->GetUnitParent();
	if (!pack)
		return 0;
//TJL 11/23/03 More division by 2 removal
	//minSpeed = u->GetCruiseSpeed()/2.0F;
	minSpeed = (float)u->GetCruiseSpeed();
	maxSpeed = (float)u->GetMaxSpeed();

#ifdef DEBUG
	counttanker++;
	MonoPrint("%d TankerWaypoints created",counttanker);
#endif

	// Fix the offset for any fixed wps (time should == 0 for any wps needing setting)
	if (mis->tot_type == TOT_TAKEOFF)
		{
		// Fixed Takeoff time
		tw = fw;
		tw->SetWPTimes(mis->tot);
		}
	else if (mis->tot_type == TOT_ENROUTE)
		{
		// Fixed enroute time
		tw = fw;
		tw->SetWPTimes(mis->tot);
		}
	else if (mis->tot_type == TOT_INGRESS)
		{
		// Fixed ingrss time
		tw = fw;
		while (tw && !(tw->GetWPFlags() & WPF_IN_PACKAGE))
			tw = tw->GetNextWP();
		if (tw)
			tw->SetWPTimes(mis->tot);
		}
	while (w)
		{
		if (w->GetWPFlags() & WPF_TARGET && !tw)
			{
			tw = w;
			w->SetWPTimes(mis->tot);
			}
		if (w->GetWPFlags() & WPF_TURNPOINT && !tw)
			tw = w;
		// Lock time in some cases - only lock target waypoint if we have to refuel somewhere..
		w->UnSetWPFlag(WPF_SPEED_LOCKED);
		if (/*w->GetWPAction() == WP_TAKEOFF ||*/ (w->GetWPFlags() & WPF_TARGET) /*|| (w->GetWPFlags() & WPF_ASSEMBLE)*/)
			w->SetWPFlag(WPF_TIME_LOCKED);
		w = w->GetNextWP();
		}

	// Now let's try and set any times we're allowed to (target to landing first)
	if (!tw)
		tw = fw;
	if (!type) // egress -> adjust target to land waypoint ; time = takeoff time
	{
	mission_time = tw->GetWPDepartureTime();
	tw->GetWPLocation(&x,&y);
	ShiAssert (mission_time > 0.0F);
	w = tw->GetNextWP();
	while (w)
		{
		w->GetWPLocation(&nx,&ny);
		dist = Distance(x,y,nx,ny);
		speed = w->GetWPSpeed();
		mission_time += TimeToArrive(dist,speed);
		// If we have the refuel waypoint, set a new departure time
		if (w->GetWPAction() == WP_REFUEL)
		{
			w->SetWPArrive(mission_time);
			mission_time += 3 * CampaignMinutes;
			w->SetWPDepartTime(mission_time);	// 3 minutes to refuel
		}
		else w->SetWPTimes(mission_time);
		// Set package turn point time, as a secondary syncronization point
		if (w->GetWPFlags() == WPF_TURNPOINT && ((Package)pack)->GetTPTime() < 1.0F)
			((Package)pack)->SetTPTime (mission_time);
		mission_time += w->GetWPStationTime();
		w = w->GetNextWP();
		x = nx; y = ny;
		}
	if (mis->flags & REQF_TE_MISSION)
		return mission_time - time;
	if (mission_time < TheCampaign.CurrentTime)
		return (long)time - (long)TheCampaign.CurrentTime;
	if (mission_time > time)
		return mission_time - time;
	return 0;
	}
	else	// ingress -> adjust takeoff to target waypoint ; time = landing time
	{
	// Backwards, from target to takeoff
	mission_time = tw->GetWPArrivalTime();
	tw->GetWPLocation(&x,&y);
	w = tw->GetPrevWP();
	while (w)
		{
		w->GetWPLocation(&nx,&ny);
		dist = Distance(x,y,nx,ny);
		speed = w->GetNextWP()->GetWPSpeed();
		mission_time -= TimeToArrive(dist,speed);
		mission_time -= w->GetWPStationTime();
		// If we have the refuel waypoint, set a new departure time
		if (w->GetWPAction() == WP_REFUEL)
			w->SetWPDepartTime(mission_time + 3 * CampaignMinutes);	// 3 minutes to refuel
		w->SetWPTimes(mission_time);
		w = w->GetPrevWP();
		x = nx; y = ny;
		}
	}
	if (mis->flags & REQF_TE_MISSION)
		return time - mission_time;
	if (mission_time < TheCampaign.CurrentTime)
		return (long)mission_time - (long)TheCampaign.CurrentTime;
	if (time > mission_time)
		return time - mission_time;
	return 0;
	}


// ====================================================
// Local Support Functions
// ====================================================

// This will check the path between two waypoints, and adjust by adding waypoints if necessary.
WayPoint CheckSafePath (WayPoint w, WayPoint nw, Flight flight)
	{
	// If we're not a lead flight, we just follow the waypoints we've been given
	if (!flight->GetUnitMissionID())
		{
		int threats = ScoreThreatsOnWPLeg(w,nw,flight->GetTeam(),TT_MAX);

		if (threats > MIN_AVOID_THREAT)
			{
			// Try to find a way around
			SetCurrentAltitude();
			if (!FindSafePath(w,nw,flight))
				return NULL;
			}
		}
	return nw;
	}

// This will return the TOTAL or MAX or AVERAGE threat between two waypoints
int ScoreThreatsOnWPLeg (WayPoint w1, WayPoint w2, Team who, int type)
	{
	GridIndex   x,y,x1,y1,x2,y2;
	float		xd,yd,d;
	int			step,steps=0,threat,dist,worst=0,al;

	// Check for threats along this leg
	w1->GetWPLocation(&x1,&y1);
	w2->GetWPLocation(&x2,&y2);
	d = Distance(x1,y1,x2,y2);
//	al = GetAltitudeLevel(sCurrentAlt);
	if (w2->GetWPFlags() & WPF_HOLDCURRENT)
		al = GetAltitudeLevel(w1->GetWPAltitude());
	else
		al = GetAltitudeLevel(w2->GetWPAltitude());

	// Traverse the waypoint leg
	xd = yd = 0.0;
	ShiAssert(d);
	if (d)
	{
		xd = (float)(x2-x1)/d;
		yd = (float)(y2-y1)/d;
	}

	dist = FloatToInt32(d);
	for (step=0; step<=dist; step+=MAP_RATIO)
		{
		x = x1 + (GridIndex)(xd*step + 0.5F);
		y = y1 + (GridIndex)(yd*step + 0.5F);
		// Check threats
		threat = ScoreThreatFast (x,y,al,who);
		if (type == TT_MAX && threat > worst)
			worst = threat;
		else if (type == TT_TOTAL || type == TT_AVERAGE)
			worst += threat;
		steps++;
		}
	if (type == TT_AVERAGE)
		worst /= steps;
	return worst;
	}

// This will return the best altitude (threat wise) in the range specified at (and around the area specified)
int CheckBestAltitude(GridIndex tx, GridIndex ty, Team who, int min, int max, int try_for, int type)
	{
	GridIndex   x,y;
	int			a,ca,la=1,ha=ALT_LEVELS,bests=9999,threat;
	int			score[ALT_LEVELS] = { 0 };
	int			bestLevel,d;
	
	// Some basic stuff
	if (min == max)
		return max;
	bestLevel = 2;

	// Determine our bounds
	for (a=1; a<ALT_LEVELS; a++)
		{
		if (MaxAltAtLevel[a] < min)
			la = a+1;
		if (MinAltAtLevel[a] > max && ha == ALT_LEVELS)
			ha = a;
		}
	if (la == ha-1)
		{
		if (la == GetAltitudeLevel(try_for))
			return try_for;
		else
			return max;
		}

	// Check this spot and 4 adjancent spots
	for (d=0; d<=Here; d+=2)
		{
		x = tx + dx[d] * 10;
		y = ty + dy[d] * 10;
		// Check each level
		for (a=la; a<ha; a++)
			{
			threat = ScoreThreatFast (x,y,a,who);
			if (type == TT_TOTAL || type == TT_AVERAGE)
				score[a] += threat;
			else if (type == TT_MAX && threat > score[a])
				score[a] = threat;
			}
		}

	// Average the scores, if that's what we're looking for
	if (type == TT_AVERAGE)
		{
		for (a=la; a<ha; a++)
			score[a] = score[a] / 5;
		}

	// Bonus for being at or near the altitude we're trying for
	ca = GetAltitudeLevel(try_for);
	score[ca] -= 20;
	if (ca-1 >= la)
		score[ca-1] -= 10;
	if (ca+1 < ha)
		score[ca+1] -= 10;

	// Find the best level
	for (a=la; a<ha; a++)
		{
		if (score[a] < bests)
			{
			bests = score[a];
			bestLevel = a;
			}
		}

	// Find the best altitude
	if (bestLevel == GetAltitudeLevel(try_for))
		return try_for;
	else if (bestLevel == GetAltitudeLevel(max))
		return max;
	else if (bestLevel == GetAltitudeLevel(min))
		return min;
	else
		return GetAltitudeFromLevel (bestLevel, tx+ty);
	}

#ifdef DEBUG
float gAvgPasses=0.0F;
int gTries=0;
#endif

// This creates a waypoint path between two waypoints, attempting to avoid threats when possible
int FindSafePath(WayPoint w1, WayPoint w2, Flight flight)
	{
	PathClass   path;
	GridIndex   x,y,nx,ny;
	WayPoint    w;
	MoveType	moveType = Air;
	int			passes = 0;

	// Set up our data
	QuickSearch = MAP_RATIO*2;                       // Use fast path routines
	w1->GetWPLocation(&x,&y);
	w2->GetWPLocation(&nx,&ny);
	w = w1;
	if (flight->IsHelicopter())
		moveType = LowAir;
	moveAlt = GetAltitudeLevel(sCurrentAlt);
	maxSearch = AIR_PATH_MAX;

	// Check high altitude
	// Loop until we find a full path to our next waypoint
	while (x != nx || y != ny)
		{
		if (GetGridPath(&path,x,y,nx,ny,moveType,flight->GetTeam(),PATH_ENEMYCOST) >= 0)
			w = FillAirPath(&path,&x,&y,nx,ny,w);
		else 
			break;
		if (passes++ > 1)
			{
			MonoPrint("Failed to find a path to target at %d,%d.\n",nx,ny);
			maxSearch = MAX_SEARCH;
			QuickSearch = 0;
			return 0;
			}
		}

#ifdef DEBUG
	gAvgPasses = (float)(gAvgPasses*gTries + passes) / (float)(gTries+1);
	gTries++;
#endif

	maxSearch = MAX_SEARCH;
	QuickSearch = 0;

	if (x!=nx || y!=ny)
		return 0;

	// Now let's try to eliminated unneeded waypoints
//	EliminateExcessWaypoints (w1,w2,flight->GetTeam());

	return 1;
	}
	
// This adds new waypoints to a path in order to 'segmentize' it.
WayPoint FillAirPath (Path path, GridIndex *x, GridIndex *y, GridIndex nx, GridIndex ny, WayPoint w)
	{
	CampaignHeading	lh,h;
	int				i,step=QuickSearch,steps=0;
	WayPoint		nw;

	ShiAssert(step);

	h = lh = (CampaignHeading) path->GetDirection(0);
	for (i=0; i<path->GetLength(); i++)
		{
		h = (CampaignHeading) path->GetDirection(i);

		// We trigger an add if we've moved a couple times and our heading has changed
		if (h != lh && steps > 1)
			{
			nw = new WayPointClass(*x,*y,0,0,0,0,WP_NOTHING,0);
			FinalizeFillerWayPoint(nw);
			w->InsertWP (nw);
	        w = nw;
			steps = 0;
			}
		else
			{
			lh = h;
			steps++;
			}

		*x += step * dx[h];
		*y += step * dy[h];
		}
	// Snap to our target if we're close enough
	if (QuickSearch && DistSqu(*x,*y,nx,ny) < QuickSearch*QuickSearch)
		{
		*x = nx;
		*y = ny;
		}
	return w;
	}

static const float COS_10=(float)cos(10*DTR), COS_120=(float)cos(120*DTR);

WayPoint EliminateExcessWaypoints (WayPoint w1, WayPoint w2, int who)
	{
	WayPoint	w,mw,nw;
	int         nh,mh,oh;
	GridIndex	x,y,mx,my,nx,ny;
	float		wnd,wmd,mnd,cwm,cnm;
#ifdef DEBUG
	static int	trimmed_by_angle=0,trimmed_by_threat=0;
#endif

	w = w1;						// Starting waypoint for this check
	mw = w->GetNextWP();		// Middle waypoint (The one we may decide to eliminate)
	nw = mw->GetNextWP();		// End waypoint
	while (w && mw && nw && w != w2 && mw != w2)
		{
		// Check to see if this is a filler way point
		if (mw->GetWPAction()==WP_NOTHING && !(mw->GetWPFlags() & WPF_CRITICAL_MASK))
			{
			// Basically, I want to trim this if:
			// a) it's co-linear with next waypoint or greater than our max angle
			w->GetWPLocation(&x,&y);
			mw->GetWPLocation(&mx,&my);
			nw->GetWPLocation(&nx,&ny);
			wnd = Distance(x,y,nx,ny);
			wmd = Distance(x,y,mx,my);
			mnd = Distance(mx,my,nx,ny);
			cwm = ((mx-x)*(nx-x) + (my-y)*(ny-y)) / (wmd * wnd);
			cnm = ((mx-nx)*(x-nx) + (my-ny)*(y-ny)) / (mnd * wnd);
			if (cwm > COS_10 || cwm < COS_120 || cnm > COS_10 || cnm < COS_120)
				{
#ifdef DEBUG
				trimmed_by_angle++;
#endif
				mw->DeleteWP ();
				}
			else
				{
				// b) It doesn't significantly reduce the threat we're exposed to
				mh = ScoreThreatsOnWPLeg (w,mw,who,TT_TOTAL) + FloatToInt32(wmd);
				oh = ScoreThreatsOnWPLeg (mw,nw,who,TT_TOTAL) + FloatToInt32(mnd);
				nh = ScoreThreatsOnWPLeg (w,nw,who,TT_TOTAL) + FloatToInt32(wnd);
				if (nh <= oh+mh)
					{
#ifdef DEBUG
					trimmed_by_threat++;
#endif
					mw->DeleteWP ();
					}
				else
					w = mw;
				}
			}
		else
			w = mw;
		// Set up for the next pass, if any
		mw = w->GetNextWP();
		nw = mw->GetNextWP();
		}
	return w;

/*
	w = w1;						// Starting waypoint for this check
	mw = w->GetNextWP();		// Middle waypoint (The one we may decide to eliminate)
	nw = mw->GetNextWP();		// End waypoint
	while (w && mw && nw && w != w2 && mw != w2)
		{
		// Check to see if this is a filler way point
		if (mw->GetWPAction()==WP_NOTHING)
			{
			// Check threats FIRST HALF
			mh = ScoreThreatsOnWPLeg (w,mw,who,TT_AVERAGE);
			// Check threats SECOND HALF
			oh = ScoreThreatsOnWPLeg (mw,nw,who,TT_AVERAGE);
			// Check threats SHORTER PATH
			nh = ScoreThreatsOnWPLeg (w,nw,who,TT_AVERAGE);
			// Take the lowest cost route
			if (nh <= oh+mh)
				{
				mw->DeleteWP ();
				mw = NULL;
				}
			else
				w = mw;
			}
		else
			w = mw;
		// Set up for the next pass, if any
		mw = w->GetNextWP();
		nw = mw->GetNextWP();
		}
	return w;
*/
	}

// This will traverse a waypoint list from w1 to w2, and find or add a wp as near to w2
// as possible while not being in enemy threat circles or within 'distance' km.
// type tells routine wether to search forwards or backwards.
WayPoint AddSafeWaypoint(WayPoint w1, WayPoint w2, int type, int distance, Team who)
	{
	WayPoint	w,nw,bw,pw=NULL;
	GridIndex	x,y,nx,ny,tx,ty,cx,cy,bx,by;
	int			guesses=0,step,owner,dist,done=0; //alt,balt=0;
	float		d,xd,yd,dsq = (float)distance*distance;

	bx = by = 0;
	w2->GetWPLocation(&tx,&ty);
	bw = w = w1;
	while (w && w != w2 && !done)
		{
		if (type)
			nw = w->GetPrevWP();
		else
			nw = w->GetNextWP();
		ShiAssert(nw);
		w->GetWPLocation(&x,&y);
		nw->GetWPLocation(&nx,&ny);
		// Check both wps, and a few points in between, depending on distance
		d = Distance(x,y,nx,ny);
		dist = FloatToInt32(d);
		xd = (float)(nx-x)/d;
		yd = (float)(ny-y)/d;
		for (step=0; step<=dist && !done; step += 10)
			{
			cx = x + (GridIndex)(xd*step + 0.5F);
			cy = y + (GridIndex)(yd*step + 0.5F);
			owner = GetOwner(TheCampaign.CampMapData,cx,cy);
			if (owner && owner != who)
				done = 1;
			else if (ScoreThreatFast (cx, cy, GetAltitudeLevel(sCruiseAlt), who) || DistSqu (cx,cy,tx,ty) < dsq)
				done = 1;
			else if (DistanceToFront(cx,cy) < distance / 2.0F)
				done = 1;
			else if (step)
				{
				// Otherwise, make this location our last best guess
				bx = cx;
				by = cy;
				bw = NULL;
				}
			else
				bw = w;											// Use existing WP if possible
			if (type)
				pw = nw;
			else
				pw = w;
			}
		w = nw;
		}
	if (bw)
		{
		// Check if we can share/co-op this waypoint
		if (bw->GetWPAction() != WP_NOTHING)
			{
			// find a decent spot for it.
			if (type)
				nw = w1->GetPrevWP();
			else
				nw = w1->GetNextWP();
			w1->GetWPLocation(&x,&y);
			nw->GetWPLocation(&nx,&ny);
			xd = d = Distance(x,y,nx,ny);
			if (xd > 5.0F)
				xd = 5.0F;
			bx = x + (GridIndex)(((float)(nx-x)/d)*xd + 0.5F);
			by = y + (GridIndex)(((float)(ny-y)/d)*xd + 0.5F);
			}
		else
			{
			bw->SetWPFlag(WPF_ASSEMBLE);
			return bw;
			}
		}
	nw = new WayPointClass(bx, by, 0, 0, 0, 0, WP_NOTHING, WPF_ASSEMBLE);
	// Add it into the wp list;
	pw->InsertWP (nw);

	// Eliminate any unneeded waypoints before or after this one.
	// KCK NOTE: This is a fairly expensive check for a possibility of eliminating one waypoint.
	// If we determine we need more speed, we can axe this.
/*	bw = nw->GetNextWP();
	if (bw && bw->GetNextWP())
		EliminateExcessWaypoints(nw,bw->GetNextWP(),who);
	bw = pw->GetPrevWP();
	if (bw)
		EliminateExcessWaypoints(bw,nw,who);
*/
	return nw;
	}

// This will traverse a wp path starting at w1 and add a waypoint at distance from w2
WayPoint AddDistanceWaypoint(WayPoint w1, WayPoint w2, int distance)
	{
	WayPoint	w,nw,bw;
	GridIndex	x,y,nx,ny,tx,ty,cx,cy;
	int			step,dist;//,alt;
	float		d,xd,yd;

	w2->GetWPLocation(&tx,&ty);
	w = w1;
	while (w && w != w2)
		{
		w->GetWPLocation(&x,&y);
		nw = w->GetNextWP();
		nw->GetWPLocation(&nx,&ny);
//		alt = nw->GetWPAltitude();
		d = Distance(x,y,nx,ny);
		dist = FloatToInt32(d);
		xd = (float)(nx-x)/d;
		yd = (float)(ny-y)/d;
		for (step=0; step<=dist; step++)
			{
			cx = x + (GridIndex)(xd*step + 0.5F);
			cy = y + (GridIndex)(yd*step + 0.5F);
			if (FloatToInt32(Distance(cx,cy,tx,ty)) <= distance)
				{
				if (step < 3)
					return w;
				bw = new WayPointClass(cx, cy, 0, 0, 0, 0, WP_NOTHING, 0);
				// Add it into the wp list;
				w->InsertWP(bw);
				return bw;
				}
			}
		w = nw;
		}
	return w2;
	}

// This 'flys' a path, and collects a list of any threats on our route
// Returns values telling wether to add sead and/or ecm.
int CheckPathThreats (Unit u)
	{
#ifdef USE_HASH_TABLES
	FalconPrivateHashTable	threats(&CampFilter);
#else
	FalconPrivateList	threats(&CampFilter);
#endif
	WayPoint		w,nw;
	int				step,retval=0,dist;
//	short			targeted=0;
//	long			target_flags;
	GridIndex		x,y,fx,fy,nx,ny;
	float			xd,yd,d;
//	CampEntity		e;

	// Set our main target as targeted
//	e = u->GetCampTarget();
//	if (e)
//		ThreatSearch[e->GetCampID()] = 2;

//	target_flags = MissionData[u->GetUnitMission()].flags;
	w = u->GetFirstUnitWP();
	nw = w->GetNextWP();
	while (w && nw)
		{
		w->GetWPLocation(&fx,&fy);
		nw->GetWPLocation(&nx,&ny);
		d = Distance(fx,fy,nx,ny);
		dist = FloatToInt32(d);
		xd = (float)(nx-fx)/d;
		yd = (float)(ny-fy)/d;
		for (step=0; step<dist; step+=5)
			{
			x = fx + FloatToInt32(xd*step + 0.5F);
			y = fy + FloatToInt32(yd*step + 0.5F);
			// Just check to see what sort of escorts we'll need, if any
			retval |= CollectThreatsFast (x,y,GetAltitudeLevel(nw->GetWPAltitude()),u->GetTeam(),FIND_NOAIR | FIND_NOMOVERS | FIND_FINDUNSPOTTED,&threats);
			// Return if we've got everything we're likely to get
			if (retval == (NEED_SEAD | NEED_ECM))
				return retval;
/*			// This was only usefull when we wanted to actually target things which got in our way.
			if (CollectThreatsFast (x,y,GetAltitudeLevel(nw->GetWPAltitude()),u->GetTeam(),FIND_NOAIR | FIND_NOMOVERS | FIND_FINDUNSPOTTED,&threats))
				{
				if (nw->GetWPAltitude() < LOW_ALTITUDE_CUTOFF)
					mt = LowAir;
				else
					mt = Air;
				if (w->GetWPArrivalTime() > 1.0F)
					time = w->GetWPArrivalTime() + TimeToArrive(Distance(fx,fy,x,y),u->GetCruiseSpeed());
				else
					time = u->GetUnitTOT();
				retval |= TargetThreats(u->GetTeam(),u->GetUnitPriority(),&threats,mt,time,target_flags,&targeted);
				}
*/
			}
		w = nw;
		nw = w->GetNextWP();
		}
	return retval;
	}

// Produces mission requests against threats in the passed list at time 'time'.
// Returns what type of escorts we'll need
int TargetThreats (Team team, int priority, F4PFList list, MoveType mt, CampaignTime time, long target_flags, short* targeted)
	{
	CampEntity			e;
	int					retval=0,strike_type=0,do_request;
	GridIndex			x,y;
	MissionRequestClass	mis;
	VuListIterator		tit(list);

	e = GetFirstEntity(&tit);
	while (e)
		{
		do_request = FALSE;
		if (e->GetAproxCombatStrength(mt,0) > 0)						// This unit can hurt us
			{
			retval |= NEED_SEAD;
			if (e->IsUnit() && e->GetSType() == STYPE_UNIT_AIR_DEFENSE && (e->GetSpotted(team) || rand() < HALF_CHANCE))
				{
				// Specifically, it's a SAM battalion
				strike_type = AMIS_SEADSTRIKE;
				mis.context = enemyAirDefense;
				do_request = TRUE;
				}
			}
		else if (e->GetDetectionRange(mt) > VisualDetectionRange[mt])	// This unit has radar
			{
			retval |= NEED_ECM;
			if (e->IsObjective())
				{
				strike_type = AMIS_OCASTRIKE;
				mis.context = enemyAirPowerRadar;
				do_request = TRUE;
				}
			else
				{
				strike_type = AMIS_SEADSTRIKE;
				mis.context = enemyAirDefense;
				do_request = TRUE;
				}
			}
		if (do_request && (target_flags & AMIS_ADDOCASTRIKE))
			{
			// Plan a type of oca Strike mission
			e->GetLocation(&x,&y);
			mis.tot = time;
			mis.who = team;
			mis.vs = e->GetTeam();
			mis.tot_type = TYPE_LE;
			mis.tx = x;
			mis.ty = y;
			mis.targetID = e->Id();
			mis.mission = strike_type;
			mis.roe_check = ROE_AIR_ATTACK;
			mis.flags = 0;
			mis.priority = priority/10;
			mis.RequestMission();
			*targeted++;
			}
		e = GetNextEntity(&tit);
		}
	return retval;
	}

// Hash Table version
int TargetThreats (Team team, int priority, FalconPrivateHashTable *list, MoveType mt, CampaignTime time, long target_flags, short* targeted)
	{
	CampEntity			e;
	int					retval=0,strike_type=0,do_request;
	GridIndex			x,y;
	MissionRequestClass	mis;
	VuHashIterator		tit(list);

	e = GetFirstEntity(&tit);
	while (e)
		{
		do_request = FALSE;
		if (e->GetAproxCombatStrength(mt,0) > 0)						// This unit can hurt us
			{
			retval |= NEED_SEAD;
			if (e->IsUnit() && e->GetSType() == STYPE_UNIT_AIR_DEFENSE && (e->GetSpotted(team) || rand() < HALF_CHANCE))
				{
				// Specifically, it's a SAM battalion
				strike_type = AMIS_SEADSTRIKE;
				mis.context = enemyAirDefense;
				do_request = TRUE;
				}
			}
		else if (e->GetDetectionRange(mt) > VisualDetectionRange[mt])	// This unit has radar
			{
			retval |= NEED_ECM;
			if (e->IsObjective())
				{
				strike_type = AMIS_OCASTRIKE;
				mis.context = enemyAirPowerRadar;
				do_request = TRUE;
				}
			else
				{
				strike_type = AMIS_SEADSTRIKE;
				mis.context = enemyAirDefense;
				do_request = TRUE;
				}
			}
		if (do_request && (target_flags & AMIS_ADDOCASTRIKE))
			{
			// Plan a type of oca Strike mission
			e->GetLocation(&x,&y);
			mis.tot = time;
			mis.who = team;
			mis.vs = e->GetTeam();
			mis.tot_type = TYPE_LE;
			mis.tx = x;
			mis.ty = y;
			mis.targetID = e->Id();
			mis.mission = strike_type;
			mis.roe_check = ROE_AIR_ATTACK;
			mis.flags = 0;
			mis.priority = priority/10;
			mis.RequestMission();
			*targeted++;
			}
		e = GetNextEntity(&tit);
		}
	return retval;
	}

/*
// Produces mission requests against threats in the passed list at time 'time'.
// Returns what type of escorts we'll need
int TargetThreats (Team team, int priority, F4PFList list, MoveType mt, CampaignTime time, long target_flags, short* targeted)
	{
	CampEntity			e;
	int					retval=0,oca,strike_type=0;
//	GridIndex			x,y;
//	MissionRequestClass	mis;
	VuListIterator		tit(list);

	e = GetFirstEntity(&tit);
	while (e)
		{
		// check for enemy stuff
		if (ThreatSearch[e->GetCampID()] < 2)
			{
			oca = 0;
			if (e->GetAproxCombatStrength(mt,0) > 0)						// This unit can hurt us
				{
				retval |= NEED_SEAD;
				if (e->IsUnit() && e->GetSType() == STYPE_UNIT_AIR_DEFENSE && (e->GetSpotted(team) || rand() < HALF_CHANCE))
					{
					// Specifically, it's a SAM battalion
					strike_type = AMIS_SEADSTRIKE;
					mis.context = enemyAirDefense;
					oca++;
					}
				}
			else if (e->GetDetectionRange(mt) > VisualDetectionRange[mt])	// This unit has radar
				{
				retval |= NEED_ECM;
				if (e->IsObjective())
					{
					strike_type = AMIS_OCASTRIKE;
					mis.context = enemyAirPowerRadar;
					oca++;
					}
				else
					{
					strike_type = AMIS_SEADSTRIKE;
					mis.context = enemyAirDefense;
					oca++;
					}
				}
			ThreatSearch[e->GetCampID()] = 2;
			if (oca && (target_flags & AMIS_ADDOCASTRIKE))
				{
				// Plan a type of oca Strike mission
				e->GetLocation(&x,&y);
				mis.tot = time;
				mis.who = team;
				mis.vs = e->GetTeam();
				mis.tot_type = TYPE_LE;
				mis.tx = x;
				mis.ty = y;
				mis.targetID = e->Id();
				mis.mission = strike_type;
				mis.roe_check = ROE_AIR_ATTACK;
				mis.flags = 0;
				mis.priority = priority/4;
				mis.RequestMission();
//				MonoPrint("Targeting unit %d\n",tu->GetUnitID());
				*targeted++;
				}
			}
		e = GetNextEntity(&tit);
		}
	return retval;
	}
	*/































/*


WayPoint EliminateExcessWaypoints (WayPoint w1, WayPoint w2, int who, int min, int max)
	{
	WayPoint	w,mw,nw;
	int         nh,mh,oh,safe=0;
	int			alt=0,alt2;

	w = w1;						// Starting waypoint for this check
	mw = w->GetNextWP();		// Middle waypoint (The one we may decide to eliminate)
	nw = mw->GetNextWP();		// End waypoint
	while (w && mw && nw && w != w2 && mw != w2)
		{
		// Check for safest Altitude
		mh = CheckBestAltitude(w,mw,who,min,max,&alt,TT_TOTAL);
		mw->SetWPAltitudeLevel(alt);
		// Check to see if this is a filler way point
		if (mw->GetWPAction()==WP_NOTHING)
			{
			// Get AA strength of shorter path
			nh = CheckBestAltitude(w,nw,who,min,max,&alt,TT_TOTAL);
			// Get AA strength of second half of longer path
			oh = CheckBestAltitude(mw,nw,who,min,max,&alt2,TT_TOTAL);
//nw->GetWPLocation(&x,&y);
//mw->GetWPLocation(&nx,&ny);
//ShowWPLeg(0,x,y,nx,ny,8);
//w->GetWPLocation(&x,&y);
//ShowWPLeg(0,x,y,nx,ny,8);
//nw->GetWPLocation(&nx,&ny);
//ShowWPLeg(0,x,y,nx,ny,9);
//MonoPrint("Costs: %d + %d = %d or %d", mh, oh, oh+mh, nh);
			// Take the lowest cost route
			if (nh <= oh+mh)
				{
				mw->DeleteWP ();
				mw = NULL;
				nw->SetWPAltitudeLevel(alt);
//ShowWPLeg(0,x,y,nx,ny,15);
//MonoPrint(" Took shorter, alt = %d\n",alt);
				}
			else
				{
				nw->SetWPAltitudeLevel(alt2);
				w = mw;
//mw->GetWPLocation(&nx,&ny);
//ShowWPLeg(0,x,y,nx,ny,15);
//nw->GetWPLocation(&x,&y);
//ShowWPLeg(0,x,y,nx,ny,15);
//MonoPrint(" Took Longer, alt = %d/%d\n",alt,alt2);
				}
			}
		else
			{
//ShowWPLeg(0,x,y,nx,ny,15);
//MonoPrint("alt = %d\n",alt);
			w = mw;
			}
//Sleep(8000);
		// Set up for the next pass, if any
		mw = w->GetNextWP();
		nw = mw->GetNextWP();
		}
	return w;
	}

*/

BOOL LoadMissionData()
{
    FILE *fp = OpenCampFile("mission", "dat", "rt");
    if (fp == NULL) return FALSE;
    char buffer[1024];

    while (fgets(buffer, sizeof buffer, fp) != NULL) {
	if (buffer[0] == '/' || buffer[0] == '\r' || buffer[0] == '\n')
	    continue;
	DWORD no, type, target, skill, misprof, tprof, tdesc, routewp, targetwp;
	int minalt, maxalt, missalt, separation, loiter;
	DWORD str, mintime, maxtime, escort, mindist, min_time, caps, flags;
	if (sscanf(buffer, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d 0x%x",
	    &no, &type, &target, &skill, &misprof, &tprof, &tdesc, &routewp, &targetwp,
	    &minalt, &maxalt, &missalt, &separation, &loiter,
	    &str, &mintime, &maxtime, &escort, &mindist, &min_time, &caps, &flags
	    ) != 22) {
	    MonoPrint("Bad line %s\n", buffer);
	    continue;
	}
	if (no < 0 || no >= sizeof(MissionData)/sizeof(MissionData[0]))
	    continue;
	MissionDataType *mp = &MissionData[no];
	mp->type = (BYTE)type;
	mp->target = (BYTE)target;
	mp->skill = (BYTE)skill;
	mp->mission_profile = (BYTE)misprof;
	mp->target_profile = (BYTE)tprof;
	mp->target_desc = (BYTE)tdesc;
	mp->routewp = (BYTE)routewp;
	mp->targetwp = (BYTE)targetwp;
	mp->minalt = minalt;
	mp->maxalt = maxalt;
	mp->missionalt = missalt;
	mp->separation = separation;
	mp->loitertime = loiter;
	mp->str = (BYTE)str;
	mp->min_time = (BYTE)mintime;
	mp->max_time = (BYTE)maxtime;
	mp->escorttype = (BYTE)escort;
	mp->mindistance = (BYTE)mindist;
	mp->min_time = (BYTE)min_time;
	mp->caps = (BYTE)caps;
	mp->flags = flags;
    }
    fclose(fp);
    return TRUE;
}

BOOL WriteMissionData()
{
	FILE *fp = OpenCampFile("mission", "dat", "wt");

    if (fp == NULL) 
		return FALSE;

	fprintf (fp, "// No Type Target skill MissionProfile TargetProfile TargetDesc "
	"RouteWP TargetWP MinAlt MaxAlt MissionAlt Separation Loiter "
	"Str MinTime MaxTime Escort MinDist Min_Time Caps Flags\n");
    for (int i = 0; i < sizeof(MissionData)/sizeof(MissionData[0]); i++) {
	MissionDataType *mp = &MissionData[i];
	fprintf(fp, 
	    "%5d %4d %6d %5d %14d %13d %10d %7d %8d %6d %6d %10d %10d %6d %3d %7d %7d %6d %7d %8d %4d 0x%08x\n",
	    i, 
	    mp->type,
	    mp->target,
	    mp->skill,
	    mp->mission_profile,
	    mp->target_profile,
	    mp->target_desc,
	    mp->routewp,
	    mp->targetwp,
	    mp->minalt,
	    mp->maxalt,
	    mp->missionalt,
	    mp->separation,
	    mp->loitertime,
	    mp->str,
	    mp->min_time,
	    mp->max_time,
	    mp->escorttype,
	    mp->mindistance,
	    mp->min_time,
	    mp->caps,
	    mp->flags);
    }
    fclose(fp);
    return TRUE;
}
