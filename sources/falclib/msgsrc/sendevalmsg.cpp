#include "MsgInc/SendEvalMsg.h"
#include "mesg.h"
#include "misseval.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "cmpclass.h"

//sfr: added here for checks
#include "InvalidBufferException.h"

extern void UpdateEvaluators(FlightDataClass *flight_data, PilotDataClass *pilot_data);

ulong	gResendEvalRequestTime = 0;

SendEvalMessage::SendEvalMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (SendEvalMsg, FalconEvent::SimThread, entityId, target, loopback)
	{
	dataBlock.data = NULL;
	dataBlock.size = 0;
	}

SendEvalMessage::SendEvalMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (SendEvalMsg, FalconEvent::SimThread, senderid, target)
	{
	dataBlock.data = NULL;
	dataBlock.size = 0;
	type;
	}

SendEvalMessage::~SendEvalMessage() {
	if (dataBlock.data)
		delete dataBlock.data;
}

int SendEvalMessage::Size () const {
    ShiAssert(dataBlock.size >= 0);
	return	(FalconEvent::Size() +	sizeof (int) + sizeof(ushort) + dataBlock.size);
}

int SendEvalMessage::Decode (VU_BYTE **buf, long *rem)
{
	long init = *rem;

	FalconEvent::Decode (buf, rem);
	memcpychk(&dataBlock.message, buf, sizeof(int), rem);
	memcpychk(&dataBlock.size, buf, sizeof(ushort), rem);
	ShiAssert(dataBlock.size >= 0);
	dataBlock.data = new uchar[dataBlock.size];
	memcpychk(dataBlock.data, buf, dataBlock.size, rem);

//	ShiAssert (size == Size());

	return init - *rem;
}

int SendEvalMessage::Encode (VU_BYTE **buf)
	{
	int		size;

    ShiAssert(dataBlock.size >= 0);
	size = FalconEvent::Encode (buf);
	memcpy (*buf, &dataBlock.message, sizeof(int));				*buf += sizeof(int);			size += sizeof(int);		
	memcpy (*buf, &dataBlock.size, sizeof(ushort));				*buf += sizeof(ushort);			size += sizeof(ushort);		
	memcpy (*buf, dataBlock.data, dataBlock.size);				*buf += dataBlock.size;			size += dataBlock.size;		

	ShiAssert (size == Size());

	return size;
	}


int SendEvalMessage::Process(uchar autodisp)
	{
	PilotDataClass		*pilot_data;
//	FlightDataClass		*flight_data;
	short				campid;
	uchar				*bufptr = dataBlock.data;
	uchar				slot,t;
	uchar				d1,d5[MAX_DOGFIGHT_TEAMS];
	short				d2,d3,d4;

	if (autodisp || Entity() == FalconLocalSession)
		return 0;

	switch (dataBlock.message)
		{
		case requestData:
			// Send all mission evaluation data owned by us.
			SendAllEvalData();
			break;
		case dogfightPilotData:
			// Copy this data into the pilot_data, if we found it.
			memcpy(&campid, bufptr, sizeof(short));				bufptr += sizeof(short);
			memcpy(&slot, bufptr, sizeof(uchar));				bufptr += sizeof(uchar);

			pilot_data = TheCampaign.MissionEvaluator->FindPilotData (campid, slot);
			if (pilot_data)
				{
				memcpy(&d1, bufptr, sizeof(uchar));				bufptr += sizeof(uchar);
				memcpy(&d2, bufptr, sizeof(short));				bufptr += sizeof(short);
				memcpy(&d3, bufptr, sizeof(short));				bufptr += sizeof(short);
				memcpy(&d4, bufptr, sizeof(short));				bufptr += sizeof(short);
				MonoPrint("Got %d/%d/%d/%d, have %d/%d/%d/%d\n",d1,d2,d3,d4,pilot_data->aa_kills,pilot_data->deaths[VS_AI],pilot_data->deaths[VS_HUMAN],pilot_data->score);
				if (d1 > pilot_data->aa_kills)
					pilot_data->aa_kills = d1;
				if (d2 > pilot_data->deaths[VS_AI])
					pilot_data->deaths[VS_AI] = d2;
				if (d3 > pilot_data->deaths[VS_HUMAN])
					pilot_data->deaths[VS_HUMAN] = d3;
				if (d4 > pilot_data->score)
					pilot_data->score = d4;
//				memcpy(&pilot_data->aa_kills, bufptr, sizeof(uchar));				bufptr += sizeof(uchar);
//				memcpy(&pilot_data->deaths[VS_AI], bufptr, sizeof(short));			bufptr += sizeof(short);
//				memcpy(&pilot_data->deaths[VS_HUMAN], bufptr, sizeof(short));		bufptr += sizeof(short);
//				memcpy(&pilot_data->score, bufptr, sizeof(short));					bufptr += sizeof(short);
				memcpy(d5, bufptr, sizeof(uchar) * MAX_DOGFIGHT_TEAMS);
				MonoPrint("Got %d/%d/%d/%d, have %d/%d/%d/%d\n",d5[1],d5[2],d5[3],d5[4],TheCampaign.MissionEvaluator->rounds_won[1],TheCampaign.MissionEvaluator->rounds_won[2],TheCampaign.MissionEvaluator->rounds_won[3],TheCampaign.MissionEvaluator->rounds_won[4]);
				for (t=0; t<MAX_DOGFIGHT_TEAMS; t++)
					{
					if (d5[t] > TheCampaign.MissionEvaluator->rounds_won[t])
						TheCampaign.MissionEvaluator->rounds_won[t] = d5[t];
					}
				bufptr += sizeof(uchar) * MAX_DOGFIGHT_TEAMS;
				}
			else
				{
				// Havn't "discovered" this player yet. Make sure to resend request
				gResendEvalRequestTime = vuxRealTime + 1000;
				}
			break;
		case dogfightFlightData:
			ShiWarning("Unimplemented case");
			break;
		case campaignPilotData:
			// Copy this data into the pilot_data, if we found it.
			memcpy(&campid, bufptr, sizeof(short));				bufptr += sizeof(short);
			memcpy(&slot, bufptr, sizeof(uchar));				bufptr += sizeof(uchar);

			pilot_data = TheCampaign.MissionEvaluator->FindPilotData (campid, slot);
			if (pilot_data)
				{
				memcpy(&pilot_data->aa_kills, bufptr, sizeof(uchar));				bufptr += sizeof(uchar);
				memcpy(&pilot_data->ag_kills, bufptr, sizeof(uchar));				bufptr += sizeof(uchar);
				memcpy(&pilot_data->an_kills, bufptr, sizeof(uchar));				bufptr += sizeof(uchar);
				memcpy(&pilot_data->as_kills, bufptr, sizeof(uchar));				bufptr += sizeof(uchar);
				memcpy(&pilot_data->deaths[VS_AI], bufptr, sizeof(short));			bufptr += sizeof(short);
				memcpy(&pilot_data->deaths[VS_HUMAN], bufptr, sizeof(short));		bufptr += sizeof(short);
				memcpy(&pilot_data->score, bufptr, sizeof(short));					bufptr += sizeof(short);
				memcpy(&pilot_data->rating, bufptr, sizeof(uchar));					bufptr += sizeof(uchar);
				}
			break;
		case campaignFlightData:
			ShiWarning("Unimplemented case");
			break;
		default:
			ShiWarning("Unimplemented case");
			break;
		}
	return 0;
	}

// ==============================================
// Supporting functions
// ==============================================

void RequestEvalData (void)
	{
	SendEvalMessage	*msg = new SendEvalMessage(vuLocalSession,FalconLocalGame);

	msg->dataBlock.message = SendEvalMessage::requestData;
	FalconSendMessage(msg,TRUE);
	}

void SendEvalData (FlightDataClass *flight_data, PilotDataClass *pilot_data)
	{
	if (FalconLocalGame)
		{
		SendEvalMessage	*msg = new SendEvalMessage(vuLocalSession,FalconLocalGame);
		int				size = 0;
		uchar			*bufptr;

		if (FalconLocalGame->GetGameType() == game_Dogfight)
			{
			msg->dataBlock.message = SendEvalMessage::dogfightPilotData;
			msg->dataBlock.data = bufptr = new uchar[50];
			memcpy(bufptr, &flight_data->camp_id, sizeof(short));				bufptr += sizeof(short);	size += sizeof(short);
			memcpy(bufptr, &pilot_data->pilot_slot, sizeof(uchar));				bufptr += sizeof(uchar);	size += sizeof(uchar);
			memcpy(bufptr, &pilot_data->aa_kills, sizeof(uchar));				bufptr += sizeof(uchar);	size += sizeof(uchar);
			memcpy(bufptr, &pilot_data->deaths[VS_AI], sizeof(short));			bufptr += sizeof(short);	size += sizeof(short);
			memcpy(bufptr, &pilot_data->deaths[VS_HUMAN], sizeof(short));		bufptr += sizeof(short);	size += sizeof(short);
			memcpy(bufptr, &pilot_data->score, sizeof(short));					bufptr += sizeof(short);	size += sizeof(short);
			// Send matchplay stuff too.
			memcpy(bufptr, TheCampaign.MissionEvaluator->rounds_won, sizeof(uchar) * MAX_DOGFIGHT_TEAMS);
			bufptr += sizeof(uchar) * MAX_DOGFIGHT_TEAMS;
			size += sizeof(uchar) * MAX_DOGFIGHT_TEAMS;
			}
		else
			{
			msg->dataBlock.message = SendEvalMessage::campaignPilotData;
			msg->dataBlock.data = bufptr = new uchar[50];
			memcpy(bufptr, &flight_data->camp_id, sizeof(short));				bufptr += sizeof(short);	size += sizeof(short);
			memcpy(bufptr, &pilot_data->pilot_slot, sizeof(uchar));				bufptr += sizeof(uchar);	size += sizeof(uchar);
			memcpy(bufptr, &pilot_data->aa_kills, sizeof(uchar));				bufptr += sizeof(uchar);	size += sizeof(uchar);
			memcpy(bufptr, &pilot_data->ag_kills, sizeof(uchar));				bufptr += sizeof(uchar);	size += sizeof(uchar);
			memcpy(bufptr, &pilot_data->an_kills, sizeof(uchar));				bufptr += sizeof(uchar);	size += sizeof(uchar);
			memcpy(bufptr, &pilot_data->as_kills, sizeof(uchar));				bufptr += sizeof(uchar);	size += sizeof(uchar);
			memcpy(bufptr, &pilot_data->deaths[VS_AI], sizeof(short));			bufptr += sizeof(short);	size += sizeof(short);
			memcpy(bufptr, &pilot_data->deaths[VS_HUMAN], sizeof(short));		bufptr += sizeof(short);	size += sizeof(short);
			memcpy(bufptr, &pilot_data->score, sizeof(short));					bufptr += sizeof(short);	size += sizeof(short);
			memcpy(bufptr, &pilot_data->rating, sizeof(uchar));					bufptr += sizeof(uchar);	size += sizeof(uchar);
			}
		msg->dataBlock.size = (short)size;
		FalconSendMessage(msg,TRUE);
		}
	}

void SendEvalData (FlightDataClass *flight_data)
	{
	return;			// TODO;

	if (FalconLocalGame)
		{
		SendEvalMessage	*msg = new SendEvalMessage(vuLocalSession,FalconLocalGame);
		int				size = 0;
		uchar			*bufptr;

		if (FalconLocalGame->GetGameType() == game_Dogfight)
			{
			msg->dataBlock.message = SendEvalMessage::dogfightFlightData;
			msg->dataBlock.data = bufptr = new uchar[50];
			// Send matchplay stuff too.
			memcpy(bufptr, TheCampaign.MissionEvaluator->rounds_won, sizeof(uchar) * MAX_DOGFIGHT_TEAMS);
			bufptr += sizeof(uchar) * MAX_DOGFIGHT_TEAMS;
			size += sizeof(uchar) * MAX_DOGFIGHT_TEAMS;
			}
		else
			{
			msg->dataBlock.message = SendEvalMessage::campaignFlightData;
			}
		msg->dataBlock.size = (short)size;
		FalconSendMessage(msg,TRUE);
		}
	flight_data;
	}

void SendAllEvalData(void)
	{
	FlightDataClass		*flight_data;
	PilotDataClass		*pilot_data;

	flight_data = TheCampaign.MissionEvaluator->flight_data;
	while (flight_data)
		{
		pilot_data = flight_data->pilot_list;
		while (pilot_data)
			{
			// This will check if we're the controlling flight
			UpdateEvaluators(flight_data, pilot_data);
//			SendEvalData(flight_data, pilot_data);
			pilot_data = pilot_data->next_pilot;
			}
		flight_data = flight_data->next_flight;
		}
	}
