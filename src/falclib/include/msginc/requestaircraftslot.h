#ifndef _REQUESTAIRCRAFTSLOT_H
#define _REQUESTAIRCRAFTSLOT_H

#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"
#include "InvalidBufferException.h"

#pragma pack (1)

class FlightClass;
typedef FlightClass* Flight;

/*
 * Message Type Request Aircraft Slot
 */

#define REQUEST_SLOT_JOIN_PLAYER		1
#define REQUEST_SLOT_JOIN_AI			2
#define REQUEST_SLOT_LEAVE				3
#define REQUEST_FLIGHT_DELETE			4
#define REQUEST_CALLSIGN_CHANGE			5
#define REQUEST_SKILL_CHANGE			6
#define REQUEST_TEAM_CHANGE				7
#define REQUEST_TYPE_CHANGE				8
#define REQUEST_UI_UPDATE				9
class UI_RequestAircraftSlot : public FalconEvent
{
   public:
      UI_RequestAircraftSlot(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      UI_RequestAircraftSlot(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~UI_RequestAircraftSlot(void);
	  virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
	  //sfr: changed to long *
	  int Decode (VU_BYTE **buf, long *rem);
      int Encode (VU_BYTE **buf);

	  class DATA_BLOCK
      {
         public:

            VU_ID requesting_session;
            uchar request_type;
            uchar game_type;
            uchar team;
			uchar current_pilot_slot;
            uchar requested_slot;
			uchar requested_skill;		// AI only
			short requested_type;		// AI only
      } dataBlock;

   protected:
		int Process(uchar autodisp);
		int ChangeFlightTeam(Flight flight);
		int ChangeFlightType(Flight flight);
		int ChangeFlightCallsign(Flight flight);
		int ChangePilotSkill(Flight flight);
		int EmptyFlightSlot(Flight flight);
		int AddFlightSlot(Flight flight);

};
#pragma pack ()

#endif
