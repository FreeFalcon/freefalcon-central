/*
 * Machine Generated include file for message "Send Eval Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 14-December-1998 at 14:17:07
 * Generated from file EVENTS.XLS by MicroProse
 */

#ifndef _SENDEVALMSG_H
#define _SENDEVALMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"
#pragma pack (1)

/*
 * Message Type Send Eval Message
 */
class SendEvalMessage : public FalconEvent
{
   public:
      enum evalMsgType {
         requestData,
         dogfightPilotData,
         dogfightFlightData,
         campaignPilotData,
         campaignFlightData};

      SendEvalMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      SendEvalMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~SendEvalMessage();
      virtual int Size() const;
		//sfr: changed to long *
	  virtual int Decode (VU_BYTE **buf, long *rem);
      virtual int Encode (VU_BYTE **buf);

      class DATA_BLOCK
      {
         public:

            unsigned int message;
            ushort size;
            uchar* data;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

class PilotDataClass;
class FlightDataClass;

extern void RequestEvalData (void);
extern void SendEvalData (FlightDataClass *flight_data, PilotDataClass *pilot_data);
extern void SendEvalData (FlightDataClass *flight_data);
extern void SendAllEvalData(void);

#endif
