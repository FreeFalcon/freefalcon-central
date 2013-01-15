#include "MsgInc/TimingMsg.h"
#include "mesg.h"
#include "ThreadMgr.h"
#include "CmpClass.h"
#include "FalcSess.h"
#include "TimerThread.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

// ===================================
// Prototypes
// ===================================

// ===================================
// Class functions
// ===================================

FalconTimingMessage::FalconTimingMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (TimingMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
	if (!target)
		return;
	
	ShiAssert(target->IsGame() && target->IsLocal());
	
	// MonoPrint ("TargetCompressionRatio %d\n", targetCompressionRatio);
	
	// Set the data
	dataBlock.targetTime = vuxGameTime;
	dataBlock.compressionRatio = (char)targetCompressionRatio;
	if (gCompressTillTime && dataBlock.targetTime > gCompressTillTime)
	{
		if (vuxGameTime < gCompressTillTime)
		{
			dataBlock.targetTime = gCompressTillTime;					// We don't want to advance time past here
		}
		else
		{
			dataBlock.targetTime = vuxGameTime;							// Unless it's already to late
		}
	}
	
//	RequestReliableTransmit ();
	RequestOutOfBandTransmit();
}

FalconTimingMessage::FalconTimingMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (TimingMsg, FalconEvent::CampaignThread, senderid, target)
{
	type;
}

FalconTimingMessage::~FalconTimingMessage(void)
{
}
extern int F4CommsLatency; 

//sfr: changed to long *
int FalconTimingMessage::Decode (VU_BYTE **buf, long *rem)
{
	long initialRem = *rem;
	#define numberofstats  30
	static long delta[numberofstats];
	static long totaltimediff =0;
	static int init = true;
	static int count =0;
	static bool must = true;
	

	if (init)
	{
	for (int i=20; i >= 0;i--)
		{
		delta[i] = 0;
		}
		init =false;
	}
	FalconEvent::Decode (buf, rem);
	memcpychk (&dataBlock, buf, sizeof (dataBlock), rem);
	
	if (!FalconLocalGame->IsLocal())
	{
//		MonoPrint
//		(
//			"Got timing message - %08x %08x %d\n",
//			vuxTargetGameTime,
//			dataBlock.targetTime,
//			vuxTargetGameTime - dataBlock.targetTime
//		);
		// me123 add the static latency
		dataBlock.targetTime += F4CommsLatency;

		if (dataBlock.targetTime >= vuxLastTargetGameTime)
		{
		int
				diff;

			diff = vuxTargetGameTime - dataBlock.targetTime;

			if ((diff < -5000) || (diff > 5000))
			{
				MonoPrint ("Update vuxTargetGameTime %08x %d\n", dataBlock.targetTime, diff);
				vuxTargetGameTime = dataBlock.targetTime;
				init = true;
			}
			else 
			{
				for (int i=numberofstats; i >= 0;i--)
				{
				if (i == 0)	delta[i] = vuxTargetGameTime - dataBlock.targetTime;
				else if (i>0) delta[i] = delta[i-1];
				totaltimediff += delta[i];
				}
				totaltimediff = totaltimediff/numberofstats;
				count++;
				if (count > numberofstats)
				{
				vuxTargetGameTime = vuxTargetGameTime - totaltimediff;
				init = true;
				count =0;
				}
			}
			vuxLastTargetGameTime = dataBlock.targetTime;
			targetGameCompressionRatio = dataBlock.compressionRatio;
		}
		
		vuxDeadReconTime = vuxRealTime + 60000;
		lastTimingMessage = vuxRealTime;
	}
	int size = initialRem - *rem;
	return size;
}

int FalconTimingMessage::Encode (VU_BYTE **buf)
{
	int size;
	
	size = FalconEvent::Encode (buf);
	memcpy (*buf, &dataBlock, sizeof (dataBlock));		*buf += sizeof (dataBlock);		size += sizeof (dataBlock);
	return size;
}

int FalconTimingMessage::Process(uchar autodisp)
{
	return 0;
	autodisp;
}

// ================================
// Global functions
// ================================
































