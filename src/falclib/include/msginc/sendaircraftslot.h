#ifndef _SENDAIRCRAFTSLOT_H
#define _SENDAIRCRAFTSLOT_H

#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

#include "InvalidBufferException.h"

#pragma pack (1)

#define REQUEST_RESULT_SUCCESS		1
#define REQUEST_RESULT_DENIED		2

/*
 * Message Type Send aircraft Slot
 */

class UI_SendAircraftSlot : public FalconEvent
{
   public:
      UI_SendAircraftSlot(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      UI_SendAircraftSlot(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~UI_SendAircraftSlot(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
	  int Decode (VU_BYTE **buf, long *rem);
      int Encode (VU_BYTE **buf);

      class DATA_BLOCK
      {
         public:

            VU_ID requesting_session;
			VU_ID game_id;
			VU_ID host_id;
			uchar result;
            uchar game_type;
            uchar team;
            uchar got_slot;
			uchar got_pilot_slot;
			uchar got_pilot_skill;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
