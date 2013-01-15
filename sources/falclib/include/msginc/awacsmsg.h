#ifndef _AWACSMSG_H
#define _AWACSMSG_H

#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type AWACSMsg
 */
class FalconAWACSMessage : public FalconEvent
{
   public:
      enum AWACSMsgCode {
         Wilco,
         Unable,
         Judy,
         RequestPicture,
         RequestHelp,
         RequestRelief,
         RequestDivert,
         RequestSAR,
         OnStation,
         OffStation,
         VectorHome,
         VectorToAltAirfield,
         VectorToTanker,
         VectorToPackage,
         VectorToThreat,
         VectorToTarget,
         GivePicture,
         CampAbortMission,
         CampAirCoverSend,
         CampAWACSOnStation,
         CampAWACSOffStation,
         CampRelievedStation,
         CampDivertCover,
         CampDivertIntercept,
         CampDivertCAS,
         CampDivertOther,
         CampDivertDenied,
         CampEndDivert,
         CampAircraftLaunch,
         CampInterceptCourse,
         CampFriendliesEngaged,
         DeclareAircraft,
	  	 VectorToCarrier}; // M.N.

      FalconAWACSMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconAWACSMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconAWACSMessage(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
	  //sfr: changed to long *
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
            size += sizeof (dataBlock);
            return size;
         };
      class DATA_BLOCK
      {
         public:

            VU_ID caller;
            unsigned int type;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
