/***************************************************************************\
    Weather.h
    Miro "Jammer" Torrielli
    209Nov03

	- And then there was light
\***************************************************************************/

#ifndef WEATHER_H
#define WEATHER_H

#include "MsgInc\WeatherMsg.h"
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "RealWeather.h"

class WeatherClass : public RealWeather
{
public:
	WeatherClass();
	~WeatherClass();
	void Setup();
	void Cleanup();
	void Init(bool instantAction = false);
	void UpdateCondition(int condition,bool bForce=false);
	void UpdateWeather();
	int CampLoad(char *name,int type);
	int Save(char *name);
	void SendWeather(VuTargetEntity* dest);
	void ReceiveWeather(FalconWeatherMessage* message);
	int GetCloudCover(GridIndex x,GridIndex y);
	int GetCloudLevel(GridIndex x,GridIndex y);
	void SetCloudCover(GridIndex x,GridIndex y, int cov);
	void SetCloudLevel(GridIndex x,GridIndex y, int lev);
	float WindSpeedInFeetPerSecond(const Tpoint *pos);
	float WindHeadingAt(const Tpoint *pos);
	virtual float TemperatureAt(const Tpoint *pos);

public:
	CampaignTime lastCheck;
	int weatherDay,stratusBase,cumulusBase;
	float turbFactor,temperature,contrailLow,contrailHigh;
	BOOL lockedCondition,needsWeatherRefresh,unlockableCondition;

protected:
	BOOL sendClouds;
	SLONG contrailBase;
	int tempMin,tempMed,tempMax,windMin,windMed,windMax,wHdgThresh,condCounter;
};

#endif //WEATHER_H
