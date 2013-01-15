#include "falclib.h"
#include "falcsess.h"
#include "falcmesg.h"
#include "mesg.h"
#include "acmi/src/include/acmirec.h"
#include "msginc/radiochattermsg.h"
#include "falcent.h"
#include "team.h"
#include "Find.h"
#include "vuevent.h"

//#define F4_DEBUG_COMMS
static int monoall = 3;

void FalconSendMessage (VuMessage* theEvent, BOOL reliableTransmit)
{
	/* Why did you do this ?
	   if (theEvent->Type() == 75)
	   {
	   delete theEvent;
	   return;
	   }
	 */
	ShiAssert(theEvent);
	if(theEvent->Type() == RadioChatterMsg)
	{
		FalconRadioChatterMessage *radioMessage = (FalconRadioChatterMessage*)theEvent;

		if(radioMessage->dataBlock.message < 0 || radioMessage->dataBlock.message >= LastComm)
		{
			ShiWarning("Bad radio message");
			MonoPrint("Dropping Chatter Message ID: %d \n", radioMessage->dataBlock.message );
			delete theEvent;
			return;
		}

		FalconEntity *from = 
			(FalconEntity*)vuDatabase->Find(((FalconRadioChatterMessage*)theEvent)->dataBlock.from);

		if(from){
			int friendly = FALSE, inrange = FALSE;
			FalconSessionEntity		*session = NULL;
			FalconEntity			*player = NULL;
			VuSessionsIterator		sit(FalconLocalGame);
			session = (FalconSessionEntity*) sit.GetFirst();
			while (session && (!friendly || !inrange))
			{
				if(session->GetPlayerEntity())
					player = (FalconEntity*)session->GetPlayerEntity();
				else if(session->GetPlayerFlight())
					player = (FalconEntity*)session->GetPlayerFlight();
				else if(session->GetPlayerSquadron())
					player = (FalconEntity*)session->GetPlayerSquadron();
				else
				{
					session = (FalconSessionEntity*) sit.GetNext();
					continue;
				}

				//the sender is friendly to at least one player
				if(GetTTRelations(player->GetTeam(), from->GetTeam()) <= Friendly)
					friendly = TRUE;

				//the sender is within radio range of at least one player
				if(DistSqu(from->XPos(),from->YPos(),player->XPos(),player->YPos()) < MAX_RADIO_RANGE*MAX_RADIO_RANGE)
					inrange = TRUE;

				session = (FalconSessionEntity*) sit.GetNext();
			}

			if(!friendly || !inrange)
			{
				//				MonoPrint("Dropping Chatter Message ID: %d  Friendly:%d  In Range:%d\n", radioMessage->dataBlock.message, friendly, inrange );
				delete theEvent;
				return;
			}
		}
	}


	if (reliableTransmit)
		theEvent->RequestReliableTransmit();

	int printit = 0;
	if (monoall == 0 ||
					((theEvent->Flags() & VU_RELIABLE_MSG_FLAG) && monoall == 1) ||
					((theEvent->Flags() & VU_OUT_OF_BAND_MSG_FLAG)&& monoall == 2) ||
					((theEvent->Flags() & VU_RELIABLE_MSG_FLAG) && 
					 (theEvent->Flags() & VU_OUT_OF_BAND_MSG_FLAG) && monoall == 3)
	   )
		printit = 1;

	if (theEvent->Target() == vuLocalSessionEntity){ printit = 0; }

#define Monoprintmessages 
#ifdef Monoprintmessages
#ifndef DAVE_DBG
#ifdef F4_DEBUG_COMMS
	switch (theEvent->Type())
	{
			case DamageMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)        MonoPrint ("DamageMsg ");
#endif

					break;
			case WeaponFireMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)        MonoPrint ("WeaponFireMsg ");
#endif

					break;
			case CampWeaponFireMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)       MonoPrint ("CampWeaponFireMsg ");
#endif

					break;
			case CampMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)       MonoPrint ("CampMsg ");
#endif

					break;
			case SimCampMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)       MonoPrint ("SimCampMsg ");
#endif

					break;
			case UnitMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)      MonoPrint ("UnitMsg ");
#endif

					break;
			case ObjectiveMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)     MonoPrint ("ObjectiveMsg ");
#endif

					break;
			case UnitAssignmentMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("UnitAssignmentMsg ");
#endif

					break;
			case SendCampaignMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)    MonoPrint ("SendCampaignMsg ");
#endif

					break;
			case TimingMsg:
#ifdef F4_DEBUG_COMMS
					//      MonoPrint ("TimingMsg ");
#endif

					break;
			case CampTaskingMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)    MonoPrint ("CampTaskingMsg ");
#endif

					break;
			case AirTaskingMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("AirTaskingMsg ");
#endif

					break;
			case GndTaskingMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("GndTaskingMsg ");
#endif

					break;
			case NavalTaskingMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("NavalTaskingMsg ");
#endif

					break;
			case TeamMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)    MonoPrint ("TeamMsg ");
#endif

					break;
			case WingmanMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("WingmanMsg ");
#endif

					break;
			case AirAIModeChange:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("AirAIModeChange ");
#endif
					break;
			case MissionRequestMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("MissionRequestMsg ");
#endif

					break;
			case DivertMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)  MonoPrint ("DivertMsg ");
#endif

					break;
			case WeatherMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("WeatherMsg ");
#endif

					break;
			case MissileEndMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("MissileEndMsg ");
#endif

					break;
			case AWACSMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("AWACSMsg ");
#endif

					break;
			case FACMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)    MonoPrint ("FACMsg ");
#endif

					break;
			case ATCMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("ATCMsg ");
#endif

					break;
			case DeathMessage:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("DeathMessage ");
#endif

					break;
			case CampEventMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("CampEventMsg  ");
#endif

					break;
			case LandingMessage:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("LandingMessage ");
#endif

					break;
			case ControlSurfaceMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("ControlSurfaceMsg ");
#endif

					break;
			case SimDataToggle:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("SimDataToggle ");
#endif

					break;
			case RequestDogfightInfo:
#ifdef F4_DEBUG_COMMS
					if (printit)  MonoPrint ("RequestDogfightInfo ");
#endif

					break;
			case SendDogfightInfo:
#ifdef F4_DEBUG_COMMS
					if (printit)    MonoPrint ("SendDogfightInfo ");
#endif

					break;
			case RequestAircraftSlot:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("RequestAircraftSlot ");
#endif

					break;
			case SendAircraftSlot:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("SendAircraftSlot ");
#endif

					break;
			case GraphicsTextDisplayMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("GraphicsTextDisplayMsg ");
#endif

					break;
			case AddSFXMessage:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("AddSFXMessage ");
#endif

					break;
			case SendPersistantList:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("SendPersistantList ");
#endif

					break;
			case SendObjData:
#ifdef F4_DEBUG_COMMS
					if (printit)    MonoPrint ("SendObjData ");
#endif

					break;
			case SendUnitData:
#ifdef F4_DEBUG_COMMS
					if (printit)    MonoPrint ("SendUnitData ");
#endif

					break;
			case RequestCampaignData:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("RequestCampaignData ");
#endif

					break;
			case SendChatMessage:
#ifdef F4_DEBUG_COMMS
					if (printit)    MonoPrint ("SendChatMessage ");
#endif

					break;
			case TankerMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("TankerMsg ");
#endif

					break;
			case EjectMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("EjectMsg ");
#endif

					break;
			case TrackMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("TrackMsg ");
#endif

					break;
			case CampDataMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)    MonoPrint ("CampDataMsg ");
#endif

					break;
			case VoiceDataMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("VoiceDataMsg ");
#endif

					break;
			case RadioChatterMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)    MonoPrint ("RadioChatterMsg ");
#endif
					break;
			case PlayerStatusMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("PlayerStatusMsg ");
#endif
					break;
			case LaserDesignateMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("LaserDesignateMsg ");
#endif
					break;
			case ATCCmdMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("ATCCmdMsg ");
#endif
					break;
			case DLinkMsg:
#ifdef  F4_DEBUG_COMMS
					if  (printit)  MonoPrint ("DLinkMsg ");
#endif
					break;
			case RequestObject:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("RequestObject ");
#endif
					break;
			case RegenerationMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("RegenerationMsg ");
#endif
					break;
			case RequestLogbook:
#ifdef F4_DEBUG_COMMS
					if (printit)    MonoPrint ("RequestLogbook ");
#endif
					break;
			case SendLogbook:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("SendLogbook ");
#endif
					break;
			case SendImage:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("SendImage ");
#endif
					break;
			case FalconFlightPlanMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("FalconFlightPlanMsg ");
#endif
					break;
			case SimDirtyDataMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)  MonoPrint ("SimDirtyDataMsg ");
#endif
					break;
			case CampDirtyDataMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("CampDirtyDataMsg ");
#endif
					break;
			case CampEventDataMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("CampEventDataMsg ");
#endif
					break;
			case SendVCMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)  MonoPrint ("SendVCMsg ");
#endif
					break;
			case SendUIMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("SendUIMsg ");
#endif
					break;
			case SendEvalMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)  MonoPrint ("SendEvalMsg ");
#endif
					break;

#if 1
			case SimPositionUpdateMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)   MonoPrint ("SimPositionUpdateMsg ");
#endif
					break;
			case SimRoughPositionUpdateMsg:
#ifdef F4_DEBUG_COMMS
					if (printit)  MonoPrint ("SimRoughPositionUpdateMsg ");
#endif
					break;
#endif

	}
	if ((printit) && theEvent->Type() != 30)
		MonoPrint("Sent Message: time %d,  Keepalive %d, LowPrio %d , reliable %d, oob %d, , size %d, target %d\n" ,
						vuxGameTime,
						theEvent->Flags() & VU_KEEPALIVE_MSG_FLAG,
						theEvent->Flags() & 0xf0,
						theEvent->Flags() & VU_RELIABLE_MSG_FLAG,
						theEvent->Flags() & VU_OUT_OF_BAND_MSG_FLAG ,
						theEvent->Size (),
						theEvent->Target());
#endif
#endif
#endif

#define MonoprintMessageStats 0
#if MonoprintMessageStats
	static VU_TIME laststatcound =0;
	static int sendreliable = 0;
	static int msgoob = 0;
	static int sendtotal = 0;
	static int TOTsendreliable = 0;
	static int TOToob = 0;
	static int TOTsendtotal = 0;
	static int msgcount = 0;
	static int count = 0;
	msgcount ++;
	if (theEvent->Flags() & VU_RELIABLE_MSG_FLAG) 
	{
		sendreliable += theEvent->Size ();
		TOTsendreliable += theEvent->Size ();
	}
	if (theEvent->Flags() & VU_OUT_OF_BAND_MSG_FLAG) 
	{
		msgoob += theEvent->Size();
		TOToob += theEvent->Size();
	}

	sendtotal += theEvent->Size ();
	TOTsendtotal += theEvent->Size ();

	if (vuxGameTime > laststatcound + 1000)
	{
		count ++;
		//MonoPrint("reliable %d, oob %d, total %d \n", sendreliable, msgoob, sendtotal);
		//MonoPrint("TOTreliable %d, TOToob %d, TOTtotal %d \n", TOTsendreliable, TOToob, TOTsendtotal);
		if ((printit) && count > 5 && theEvent->Type() != 30){
			MonoPrint("Avgreliable %d, Avgoob %d, Avgtotal %d mescount %d \n",
				(TOTsendreliable/count), (TOToob/count), (TOTsendtotal/count),msgcount);
		}
		sendreliable = 0;
		msgoob = 0;
		sendtotal = 0;
		laststatcound =vuxGameTime;
		if (count > 60)	{
			count = 0;
			TOTsendreliable= 0;
			TOToob= 0;
			TOTsendtotal= 0;
		}
	}


#endif

	VuMessageQueue::PostVuMessage (theEvent);
}

// =============================================
// Falcon Event
// =============================================

FalconEvent::FalconEvent (VU_MSG_TYPE type, HandlingThread threadID, VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : VuMessage (type, entityId, target, FALSE)
{
	// KCK NOTE: VU will not send a message to ourselves unless loopback is set to false - regardless of the target.
	// Also, it will send to ourselves if loopback is set to true - again, regarless of the target. 
	// So we'd better be sure to set it correctly.
	// I'd suggest always setting loopback to true unless you know you don't want to get the message,
	// because I'll explicitly set it to FALSE here if you're not included in the target.
	if (loopback)
	{
		//		if (!target && !FalconLocalGame)
		//			RequestLoopback();
		if (!target)
			loopback = FALSE;
		else if (target->IsGroup() && !((VuGroupEntity*)target)->SessionInGroup(FalconLocalSession))
			loopback = FALSE;
		else if (target->IsSession() && target != FalconLocalSession)
			loopback = FALSE;
		else
			RequestLoopback();
	}
	handlingThread = threadID;
}

FalconEvent::FalconEvent (VU_MSG_TYPE type, HandlingThread threadID, VU_ID senderid, VU_ID target) : VuMessage (type, senderid, target)
{
	handlingThread = threadID;
}

FalconEvent::~FalconEvent(void)
{
}

int FalconEvent::Size() const
{
	return FalconEvent::LocalSize() + VuMessage::Size();
}

int FalconEvent::LocalSize() const
{
	return sizeof(HandlingThread);
}

int FalconEvent::Decode (VU_BYTE **buf, long *rem) {
	//	long start = (long) *buf;
	long init = *rem;

	VuMessage::Decode (buf, rem);
	memcpychk(&handlingThread, buf, sizeof (HandlingThread), rem);

	//	ShiAssert ( size == (long) *buf - start );

	return init - *rem;
}

int FalconEvent::Encode (VU_BYTE **buf)	{
	int size;
	long start = (long) *buf;

	size = VuMessage::Encode (buf);
	memcpy (*buf, &handlingThread, sizeof (HandlingThread));
	*buf += sizeof (HandlingThread);

	size += FalconEvent::LocalSize();

	ShiAssert ( size == (long) *buf - start );

	return size;
}

int FalconEvent::Activate(VuEntity *theEntity)
{
	unsigned char* buffer;
	unsigned char* savePos;
	EventIdData idData;

	VuMessage::Activate(theEntity);

	// Only record sim events to disk if ACMI is recording
	if (F4EventFile && gACMIRec.IsRecording() && handlingThread == SimThread && Type() != ControlSurfaceMsg)
	{
		idData.size = (unsigned short)Size();
		idData.type = Type();
		buffer = new unsigned char[idData.size];
		savePos = buffer;
		Encode(&buffer);
		fwrite (&idData, sizeof (EventIdData), 1, F4EventFile);
		fwrite (savePos, idData.size, 1, F4EventFile);
		delete [] savePos;
	}
	return 0;
}

// ====================================
// Message Filter stuff
// ====================================

#if VU_USE_ENUM_FOR_TYPES
FalconMessageFilter::FalconMessageFilter(FalconEvent::HandlingThread theThread, bool processVu) :
	filterThread(theThread), processVu(processVu)
{
}
#else
FalconMessageFilter::FalconMessageFilter(FalconEvent::HandlingThread theThread, ulong vuMessageBits){
{
	filterThread = theThread;
#if MF_DONT_PROCESS_DELETE
	vuFilterBits = vuMessageBits;
#else
	// KCK: All threads must handle delete events
	vuFilterBits = vuMessageBits | VU_DELETE_EVENT_BITS;
#endif
}
#endif

FalconMessageFilter::~FalconMessageFilter(){
}

VU_BOOL FalconMessageFilter::Test(VuMessage *event) const {
	uchar retval = TRUE;

	if (event->Type() > VU_LAST_EVENT){
		// This is a Falcon Event
		if ((((FalconEvent*)event)->handlingThread & filterThread) == 0){
			// message not intended for this thread
			retval = FALSE;
		}
	}
	else {
#if VU_USE_ENUM_FOR_TYPES
		// This is a Vu Event. Compare vs filter bits
		// sfr: fixed shift adding -1
		if (!processVu){
			retval = FALSE;
		}
#else
		// This is a Vu Event. Compare vs filter bits
		// sfr: fixed shift adding -1
		if (! 
			((1 << (event->Type() - 1)) & vuFilterBits) 
		){
			retval = FALSE;
		}
#endif
	}
	return (retval);
}

VuMessageFilter* FalconMessageFilter::Copy() const {
#if VU_USE_ENUM_FOR_TYPES
	return new FalconMessageFilter(filterThread, processVu);
#else
	return new FalconMessageFilter(filterThread,vuFilterBits);
#endif
}
