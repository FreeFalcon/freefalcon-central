/*
 * Machine Generated include file for message "Request Team Info".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 01-April-1997 at 18:57:00
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#ifndef _REQUESTTEAMINFOMSG_H
#define _REQUESTTEAMINFOMSG_H

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
 * Message Type Request Team Info
 */
class FalconRequestTeamInfoMessage : public FalconEvent
{
   public:
      FalconRequestTeamInfoMessage(VU_ID entityId, VU_ID dest, VU_BYTE routing);
      FalconRequestTeamInfoMessage(VU_ID entityId, VU_BYTE routing);
      ~FalconRequestTeamInfoMessage(void);
      int Size (void) { return sizeof(dataBlock) + FalconEvent::Size();};
	  int Decode (VU_BYTE **buf, long *rem)
	  {
		  long init = *rem;

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
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
