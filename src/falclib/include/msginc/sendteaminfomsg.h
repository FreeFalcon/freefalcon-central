/*
 * Machine Generated include file for message "Send Team Info".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 01-April-1997 at 18:57:00
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#ifndef _SENDTEAMINFOMSG_H
#define _SENDTEAMINFOMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

//sfr: for chks
#include "InvalidBufferException.h"
using std::memcpychk;

#pragma pack (1)

/*
 * Message Type Send Team Info
 */
class FalconSendTeamInfoMessage : public FalconEvent
{
   public:
      FalconSendTeamInfoMessage(VU_ID entityId, VU_ID dest, VU_BYTE routing);
      FalconSendTeamInfoMessage(VU_ID entityId, VU_BYTE routing);
      ~FalconSendTeamInfoMessage(void);
      int Size (void) { return sizeof(dataBlock) + FalconEvent::Size();};
	  int Decode (VU_BYTE **buf, long *rem)
	  {
		  int init = *rem;

		  FalconEvent::Decode (buf, rem);
		  memcpychk(&dataBlock, buf, sizeof (dataBlock), rem);
		  return init - *rem;
	  };
      int Encode (VU_BYTE **buf)
         {
         int size;

            size = FalconEvent::Encode (buf);
            memcpy (*buf, &dataBlock, sizeof (dataBlock));
            *buf += sizeof (dataBlock);
            return size;
         };
      class DATA_BLOCK
      {
         public:

            ulong campTime;
            ushort whoDidIt;
            short stuff;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
