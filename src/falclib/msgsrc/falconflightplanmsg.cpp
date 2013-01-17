/*
 * Machine Generated source file for message "Flight Plan Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 10-November-1998 at 16:18:21
 * Generated from file EVENTS.XLS by .
 */

#include "InvalidBufferException.h"
#include "MsgInc/FalconFlightPlanMsg.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "flight.h"

//sfr: added here for checks
#include "InvalidBufferException.h"

FalconFlightPlanMessage::FalconFlightPlanMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (FalconFlightPlanMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
	dataBlock.data = NULL;
	dataBlock.size = 0;
	//me123	RequestOutOfBandTransmit ();
	RequestReliableTransmit ();
}

FalconFlightPlanMessage::FalconFlightPlanMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (FalconFlightPlanMsg, FalconEvent::CampaignThread, senderid, target)
{
	dataBlock.data = NULL;
	dataBlock.size = 0;
	type;
}

FalconFlightPlanMessage::~FalconFlightPlanMessage(void)
{
	if (dataBlock.data){
		delete dataBlock.data;
	}
	dataBlock.data = NULL;
}

int FalconFlightPlanMessage::Size() const { 
	ShiAssert ( dataBlock.size >= 0 );
	return sizeof (uchar) + sizeof (long) + dataBlock.size + FalconEvent::Size();
}

//int FalconFlightPlanMessage::Decode (VU_BYTE **buf, int length)
int FalconFlightPlanMessage::Decode (VU_BYTE **buf, long *rem)
{
	long init = *rem;
	FalconEvent::Decode (buf, rem);
	memcpychk(&dataBlock.type, buf, sizeof(uchar), rem);
	memcpychk(&dataBlock.size, buf, sizeof(long), rem);
	//	ShiAssert ( dataBlock.size >= 0 );
	dataBlock.data = new uchar[dataBlock.size];
	memcpychk(dataBlock.data, buf, dataBlock.size, rem);
	return init - *rem;
}

int FalconFlightPlanMessage::Encode (VU_BYTE **buf)
{
	int size;

	size = FalconEvent::Encode (buf);
	ShiAssert ( dataBlock.size >= 0 );
	memcpy (*buf, &dataBlock.type, sizeof(uchar));		*buf += sizeof (uchar);			size += sizeof (uchar);
	memcpy (*buf, &dataBlock.size, sizeof(long));		*buf += sizeof (long);			size += sizeof (long);
	memcpy (*buf, dataBlock.data, dataBlock.size);		*buf += dataBlock.size;			size += dataBlock.size;
	return size;
}

int FalconFlightPlanMessage::Process(uchar autodisp)
{
	Unit		unit = (Unit) Entity();
	VU_BYTE		*buffer = dataBlock.data;
	long		rem	= dataBlock.size;
	long		lbsfuel,planes;
	//sfr: added rem for checks


	if (autodisp || !unit || !unit->IsLocal()){
		return -1;
	}

	switch (dataBlock.type)
	{
			case squadronStores:
					short		weapon[HARDPOINT_MAX];
					unsigned char weapons[HARDPOINT_MAX];

					memcpychk(weapon,&buffer,HARDPOINT_MAX * sizeof(short), &rem);
					memcpychk(weapons,&buffer,HARDPOINT_MAX, &rem);
					memcpychk(&lbsfuel,&buffer,sizeof(long), &rem);					
					memcpychk(&planes,&buffer,sizeof(long), &rem);
					((Squadron)unit)->UpdateSquadronStores (weapon, weapons, lbsfuel, planes);
					((Squadron)unit)->MakeSquadronDirty (DIRTY_SQUAD_STORES, DDP[149].priority);
					//	((Squadron)unit)->MakeSquadronDirty (DIRTY_SQUAD_STORES, SEND_EVENTUALLY);
					break;
			case loadoutData:
					uchar				ac;
					LoadoutStruct		*loadout;
					int					i;

					memcpychk(&lbsfuel,&buffer,sizeof(long), &rem);
					memcpychk(&ac, &buffer, sizeof(uchar), &rem);
					loadout = new LoadoutStruct[ac];

					for (i=0; i<ac; i++) {
						memcpychk(loadout[i].WeaponID, &buffer, HARDPOINT_MAX * sizeof(short), &rem);
						memcpychk(loadout[i].WeaponCount, &buffer, HARDPOINT_MAX, &rem);
					}

					((Flight)unit)->SetLoadout(loadout,ac);
					((Flight)unit)->MakeFlightDirty (DIRTY_STORES, DDP[150].priority);
					break;
			case waypointData:
					unit->DecodeWaypoints(&buffer, &rem);
					unit->MakeUnitDirty (DIRTY_WP_LIST, DDP[151].priority);
					break;
			default:
					return -1;
	}
	return 0;
}

