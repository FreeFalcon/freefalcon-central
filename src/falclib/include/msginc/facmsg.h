#ifndef _FACMSG_H
#define _FACMSG_H

#include "F4vu.h"
#include "falcmesg.h"
#include "mission.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type FAC Message
 */
class FalconFACMessage : public FalconEvent
{
   public:
      enum FACMsgCode {
         CheckIn,
         Wilco,
         Unable,
         Ready,
         In,
         Out,
         RequestMark,
         RequestTarget,
         RequestBDA,
         RequestLocation,
         RequestTACAN,
         HoldAtCP,
         FacSit,
         Mark,
         AttackBrief,
         NoTargets,
         GroundTargetBr,
         BDA,
         NoBDA,
         FriendlyFire,
         ReattackQuery,
         HartsTarget,
         HartsOpen,
         ScudLaunch,
         SanitizeLZ,
         AttackMyTarget,
         SendChoppers};

      FalconFACMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconFACMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconFACMessage(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
	  //sfr: changed
	  int Decode (VU_BYTE **buf, long *rem)
	  {
		  long int init = *rem;

		  FalconEvent::Decode (buf, rem);
		  memcpychk(&dataBlock, buf, sizeof (dataBlock), rem);
		  return init  - *rem;
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
            VU_ID caller;
            unsigned int type;
            VU_ID target;
            float data1;
            float data2;
            float data3;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

#endif
