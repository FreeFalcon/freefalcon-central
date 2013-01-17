/*
 * Machine Generated include file for message "Sim Timing".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 23-April-1997 at 12:45:31
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#ifndef _SIMTIMINGMSG_H
#define _SIMTIMINGMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

#pragma pack (1)

/*
 * Message Type Sim Timing
 */

/*
class FalconSimTiming : public FalconEvent
{
   public:
      FalconSimTiming(VU_ID entityId, VU_ID dest, VU_BYTE routing);
      FalconSimTiming(VU_ID entityId, VU_BYTE routing);
      ~FalconSimTiming(void);
      int Size (void) { return sizeof(dataBlock) + FalconEvent::Size();};
      int Decode (VU_BYTE **buf, int length)
         {
         int size;

            size = FalconEvent::Decode (buf, length);
            memcpy (&dataBlock, *buf, sizeof (dataBlock));
            *buf += sizeof (dataBlock);
            size += sizeof (dataBlock);
            return size;
         };
      int Encode (VU_BYTE **buf)
         {
         int size;

            size = FalconEvent::Encode (buf);
            memcpy (*buf, &dataBlock, sizeof (dataBlock));
            *buf += sizeof (dataBlock);
            size += sizeof (dataBlock);
            return size;
         };
      class DATA_BLOCK
      {
         public:

            int paused;
            int compression;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
*/
#pragma pack ()

#endif
