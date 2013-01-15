/*
 * Machine Generated include file for message "Weather Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 01-April-1997 at 18:57:03
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */
//JAM 20Nov03

#ifndef _WEATHERMSG_H
#define _WEATHERMSG_H

#include "F4vu.h"
#include "falcmesg.h"
#include "mission.h"
#include "RealWeather.h"
#pragma pack (1)

class FalconWeatherMessage : public FalconEvent
{
public:
	FalconWeatherMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
    FalconWeatherMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
    ~FalconWeatherMessage(void);
    virtual int Size() const;
	//sfr: changed to long *
    virtual int Decode(VU_BYTE **buf, long *rem);
    virtual int Encode(VU_BYTE **buf);

    struct DATA_BLOCK {
		int weatherCondition;
		ulong lastCheck;
		float temperature;
		float windSpeed;
		float windHeading;
		float cumulusZ;
		float stratusZ;
		float stratus2Z;
		float contrailLow;
		float contrailHigh;
		float ShadingFactor;
		float weatherQuality;
    } dataBlock;

protected:
	int Process(uchar autodisp);
};

#pragma pack ()

#endif
