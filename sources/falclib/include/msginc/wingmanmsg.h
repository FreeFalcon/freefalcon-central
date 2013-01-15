#ifndef _WINGMANMSG_H
#define _WINGMANMSG_H

#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"

//sfr: checks
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type WingmanMsg
 */
class FalconWingmanMsg : public FalconEvent
{
   public:
      enum WingManCmd {
         WMSpread,
         WMWedge,
         WMTrail,
         WMLadder,
         WMStack,
         WMResCell,
         WMBox,
         WMArrowHead,
         WMFluidFour,
         WMAssignTarget,
         WMAssignGroup,
         WMShooterMode,
         WMCoverMode,
         WMWeaponsHold,
         WMWeaponsFree,
         WMBreakRight,
         WMBreakLeft,
         WMClearSix,
         WMCheckSix,
         WMPince,
         WMPosthole,
         WMChainsaw,
         WMFlex,
         WMRejoin,
         WMResumeNormal,
         WMSearchGround,
         WMSearchAir,
         WMKickout,
         WMCloseup,
         WMToggleSide,
         WMIncreaseRelAlt,
         WMDecreaseRelAlt,
         WMGiveBra,
         WMGiveStatus,
         WMGiveDamageReport,
         WMGiveFuelState,
         WMGiveWeaponsCheck,
         WMRTB,
         WMFree,
         WMPromote,
         WMRadarStby,
         WMRadarOn,
         WMBuddySpike,
         WMSkate,
         WMSSOffset,
         WMSmokeOn,
         WMSmokeOff,
         WMRaygun,
         WMLeadGearUp,
         WMLeadGearDown,
         WMLeadGearQuery,
         WMJokerFuel,
         WMBingoFuel,
         WMFumes,
         WMFlameout,
		 WMSplit,
		 WMGlue,
         WMTotalMsg,
         WMDropStores,
		 WMVic,				// 59	-> wingai.cpp AiSetFormation
		 WMFinger4,			// 60      formation number compared to this enum index decides on
		 WMEchelon,			// 61      eval index to play-> keep some free here if more
		 WMForm1,			// 62	   formations shall be added later (next command = 70)
		 WMForm2,			// 63
		 WMForm3,			// 64	   placeholder formation callbacks
		 WMForm4,			// 65
		 WMLightsOn = 70,
		 WMLightsOff = 71,
		 WMECMOn = 72,
		 WMECMOff = 73,
/*       WMBvrSingleSideOffset = 74,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
         WMBvrPince = 75,				// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
         WMBvrPursuit = 76,			// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
         WMBvrNoIntercept = 77,		// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMBvrPump = 78,				// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMBvrCrank = 79,				// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMBvrCrankRight = 80,		// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMBvrCrankLeft = 81,			// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMBvrNotch = 82,				// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMBvrNotchRight = 83,		// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMBvrNotchRightHigh = 84,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMBvrNotchLeft = 85,			// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMBvrNotchLeftHigh = 86,		// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMBvrGrind = 87,				// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMLastMsg = 88};			// WMLastMsg = 92 fake command};	
*/
		 WMPlevel1a = 74,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMPlevel2a = 75,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPlevel3a = 76,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMPlevel1b = 77,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPlevel2b = 78,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPlevel3b = 79,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMPlevel1c = 80,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPlevel2c = 81,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPlevel3c = 82,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPbeamdeploy = 83,// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPbeambeam = 84,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPwall = 85,		// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPgrinder = 86,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPwideazimuth = 87,// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPshortazimuth = 88,// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPwideLT = 89,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	     WMPShortLT = 90,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMPDefensive = 91,	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
		 WMLastMsg = 92};	// WMLastMsg = 92 fake command};	
				

      FalconWingmanMsg(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconWingmanMsg(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
	  ~FalconWingmanMsg(void);
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

            VU_ID from;
            short to;
            unsigned int command;
            VU_ID newTarget;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
