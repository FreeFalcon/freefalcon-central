//JAM 20Nov03
#include "MsgInc/WeatherMsg.h"
#include "mesg.h"
#include "CmpClass.h"
#include "Weather.h"
#include "InvalidBufferException.h"

FalconWeatherMessage::FalconWeatherMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback)
 : FalconEvent(WeatherMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
	dataBlock.weatherCondition = SUNNY;
}

FalconWeatherMessage::FalconWeatherMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target)
 : FalconEvent(WeatherMsg, FalconEvent::CampaignThread, senderid, target)
{
	dataBlock.weatherCondition = SUNNY;
}

FalconWeatherMessage::~FalconWeatherMessage(void)
{
}

int FalconWeatherMessage::Size(void) const {
	return sizeof(dataBlock) + FalconEvent::Size();
}

//sfr: changed to long *
int FalconWeatherMessage::Decode(VU_BYTE **buf, long *rem)
{
	long init = *rem;
	long start = (long)*buf;

	memcpychk(&dataBlock, buf, sizeof(dataBlock), rem);
	FalconEvent::Decode(buf, rem);

	int size = init - *rem;
	ShiAssert(size == (long)*buf - start);

	return size;
}

int FalconWeatherMessage::Encode(VU_BYTE **buf)
{
	int size = 0;

	memcpy(*buf,&dataBlock,sizeof(dataBlock));
	*buf += sizeof(dataBlock);
	size += sizeof(dataBlock);
	size += FalconEvent::Encode(buf);

	ShiAssert(size == Size());

	return size;
}

int FalconWeatherMessage::Process(uchar autodisp)
{
	if(autodisp || !TheCampaign.IsPreLoaded()) return -1;

	((WeatherClass *)realWeather)->ReceiveWeather(this);
	
	return 0;
}
