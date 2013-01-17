/*
 * Machine Generated include file for message "Weapon Fire".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 09-October-1998 at 14:17:55
 * Generated from file EVENTS.XLS by Microprose
 */

#ifndef _WEAPONFIREMSG_H
#define _WEAPONFIREMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"

//sfr: chks
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Weapon Fire
 */
class FalconWeaponsFire : public FalconEvent
{
		public:
				enum WeaponType {
					GUN,
					//       LRM,	// I don't think this is used (or handled)  SCR 10/7/98
					MRM,
					SRM,
					BMB,
					ARM,
					AGM,
					Rocket,
					Recon};

				FalconWeaponsFire(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
				FalconWeaponsFire(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
				~FalconWeaponsFire(void);
				virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
				//sfr: added long *
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

								unsigned int weaponType;
								VU_ID fEntityID;
								ushort fCampID;
								uchar fPilotID;
								ushort fIndex;
								uchar fSide;
								ushort fWeaponID;
								VU_ID fWeaponUID;
								VU_ID targetId;
								float dx;
								float dy;
								float dz;
								unsigned int gameTime;
				} dataBlock;

		protected:
				int Process(uchar autodisp);
};
#pragma pack ()

#endif
