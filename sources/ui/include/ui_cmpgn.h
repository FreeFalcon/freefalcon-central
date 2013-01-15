#ifndef _UI_CAMPAIGN_H_
#define _UI_CAMPAIGN_H_

typedef struct SquadPlayerStr SQUADRONPLAYER;

struct SquadPlayerStr
{
	VU_ID SquadronID;
	short PlayerCount;
	SQUADRONPLAYER *Next;
};

typedef struct
{
	long Action;
	long Flag;
	long ID;
	long Default;
} WP_ACTION;

enum // Flight Status List
{
	_MIS_BRIEFING=0,
	_MIS_ENROUTE,
	_MIS_INGRESS,
	_MIS_PATROL,
	_MIS_EGRESS,
	_MIS_RTB,
	_MIS_LAND,
};

#endif