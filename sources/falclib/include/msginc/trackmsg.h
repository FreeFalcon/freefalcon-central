/*
 * Machine Generated include file for message "Radar Track Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 28-September-1997 at 17:47:47
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#ifndef _TRACKMSG_H
#define _TRACKMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "falcmesg.h"
#include "mission.h"
#pragma pack (1)

enum TrackType {
	Track_None = 0,	// Never sent, but the RWR uses this one internally
	Track_Ping,		// Never sent, but the RWR uses this one internally
	Track_Unlock,
	Track_Lock,
	Track_LaunchEnd,
	Track_Launch,
	Track_SmokeOn,
	Track_SmokeOff,
	Track_JettisonAll,
	Track_JettisonWeapon,
	Track_RemoveWeapon			
};
	
/*
 * Message Type Radar Track Message
 */
class FalconTrackMessage : public FalconEvent
{
public:
	FalconTrackMessage(int reliable ,VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
	FalconTrackMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
	~FalconTrackMessage(void);
	
	virtual int Size() const { return (sizeof(unsigned int) + sizeof(VU_ID) + FalconEvent::Size()); };

	//sfr: long *	
	int Decode (VU_BYTE **buf, long *rem);
	int Encode (VU_BYTE **buf);
	
	class DATA_BLOCK
	{
	public:
		
		unsigned int trackType :16;
		unsigned int hardpoint :16;
		VU_ID id;
	} dataBlock;
	
	protected:
		int Process(uchar autodisp);
};
#pragma pack ()

#endif
