/*
 * Machine Generated include file for message "Update Settings".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 12-January-1998 at 21:13:25
 * Generated from file EVENTS.XLS by Peter
 */

#ifndef _UPDATESETTINGS_H
#define _UPDATESETTINGS_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

//sfr: checks
#include "InvalidBufferException.h"
using std::memcpychk;

#pragma pack (1)

/*
 * Message Type Update Settings
 */
class UI_UpdateSettings : public FalconEvent
{
		public:
				UI_UpdateSettings(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
				UI_UpdateSettings(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
				~UI_UpdateSettings(void);
				int Size (void) { return sizeof(dataBlock) + FalconEvent::Size();};
				//sfr: changed to long *
				int Decode (VU_BYTE **buf, long *rem) {
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
								short setting;
								long value;
				} dataBlock;

		protected:
				int Process(uchar autodisp);
};
#pragma pack ()

#endif
