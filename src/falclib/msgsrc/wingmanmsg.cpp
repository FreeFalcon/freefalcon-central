#include "MsgInc/WingmanMsg.h"
#include "mesg.h"
#include "aircrft.h"
#include "wingorder.h"

#include "flight.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"


FalconWingmanMsg::FalconWingmanMsg(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (WingmanMsg, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

FalconWingmanMsg::FalconWingmanMsg(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (WingmanMsg, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
	type;
}

FalconWingmanMsg::~FalconWingmanMsg(void)
{
   // Your Code Goes Here
}

void SendOrder(FalconWingmanMsg* p_msg, AircraftClass* p_sender, FlightClass* p_flight, int first, int total)
{
	AircraftClass* p_aircraft;
	int i;

	first--;

	for(i = first; i < first + total; i++) {
		p_aircraft = (AircraftClass*) p_flight->GetComponentEntity(i);
		if(p_aircraft && p_aircraft->IsLocal() && p_sender && p_aircraft != p_sender) {
			p_aircraft->ReceiveOrders(p_msg);
		}
	}
}

int FalconWingmanMsg::Process(uchar autodisp)
{
	FlightClass			*p_flight;
	AircraftClass		*p_from;
	int					fromIndex;

 	if (autodisp)
		return 0;

  if (Entity()) {

		p_flight		= (FlightClass*) vuDatabase->Find(EntityId());
		p_from		= (AircraftClass*) vuDatabase->Find(dataBlock.from);

		if(!p_flight || !p_from) {
			return FALSE;
		}

		fromIndex	= p_flight->GetComponentIndex(p_from);

		if(dataBlock.to == AiAllButSender) {
			SendOrder(this, p_from, p_flight, 1, 4);				// Dispatch to everyone except the guy who sent me
		}
		else if(fromIndex == AiFlightLead) {

			if(dataBlock.to == AiWingman) {
				SendOrder(this, p_from, p_flight, 2, 1);			// Dispatch to 2 only
			}
			else if(dataBlock.to == AiElement) {
				SendOrder(this, p_from, p_flight, 3, 2);			// Dispatch to 3 and 4
			}
			else if(dataBlock.to == AiFlight) {
				SendOrder(this, p_from, p_flight, 2, 3);			// Dispatch to 2, 3 and 4
			}
		}
		else if(fromIndex == AiElementLead) {
				SendOrder(this, p_from, p_flight, 4, 1);			// Dispatch to 4 only
		}
	}

	return TRUE;
}
