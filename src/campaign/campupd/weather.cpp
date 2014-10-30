/***************************************************************************\
    Weather.cpp
    Miro "Jammer" Torrielli
 20Nov03

 - And then there was light
\***************************************************************************/

#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Weather.h"
#include "math.h"
#include "F4Find.h"
#include "Entity.h"
#include "Campaign.h"
#include "Falcmesg.h"
#include "AIInput.h"
#include "F4Thread.h"
#include "CmpClass.h"
#include "MsgInc/WeatherMsg.h"
#include "falcsess.h"
#include "F4Comms.h"
#include "otwdrive.h"
#include "tmap.h"
#include "FakeRand.h"


extern int gCurrentDataVersion;
float COVersion = 0.0077f; // Cobra file version kludge

WeatherClass::WeatherClass() : RealWeather()
{
    cumulusZ = stratusZ = stratus2Z = 0.f;
    cumulusBase = stratusBase = stratus2Base = 0;
    temperature = windSpeed = windHeading = turbFactor = 0.f;
    weatherCondition = 1;
    needsWeatherRefresh = updateLighting = lockedCondition = unlockableCondition = FALSE;
}

WeatherClass::~WeatherClass()
{

}

void WeatherClass::Init(bool instantAction)
{
    condCounter = 0;
    lastCheck = Camp_GetCurrentTime();
    weatherDay = TheCampaign.GetCurrentDay();


    if ( not instantAction)
    {
        lockedCondition = TRUE;
        // Cobra - no random weather
        // UpdateCondition(min(1+rand()%4,4));
        UpdateCondition(PlayerOptions.weatherCondition);
    }

    switch (TimeOfDayGeneral())
    {
        case TOD_NIGHT:
        {
            windSpeed = (float)(windMin + (rand() % 5));
            temperature = (float)(tempMin + (rand() % 3));
            break;
        }

        case TOD_DAWNDUSK:
        {
            windSpeed = (float)(windMin + (rand() % 10));
            temperature = (float)(tempMin + (rand() % 5));
            break;
        }

        default:
        {
            windSpeed = (float)(windMed + (rand() % 20));
            temperature = (float)(min(tempMed + (rand() % 10), tempMax));
        }
    }


    windHeading = (rand() % 360) * DTR;

    if (windHeading > 2.f * PI) windHeading -= 2.f * PI;

    if (weatherCondition == INCLEMENT)
    {
        temperature *= 0.75f;
        windSpeed = windSpeed + (rand() % 20);
    }

    contrailLow = 100.f * (float)contrailBase + 100 * (rand() % 10);
    contrailHigh = max(95000.f, (float)(2 + contrailLow + 1000 * (rand() % 8))); // FRB - For the SR-71
    //contrailHigh = max(35000.f, (float)(2 + contrailLow + 1000 * (rand()%8)));

    cumulusZ = (float) - (100 * cumulusBase + 100 * (rand() % 5));

    if (weatherCondition > FAIR)
    {
        stratusZ = (float) - (100 * stratusBase + 100 * (rand() % 20));

        if (weatherCondition == INCLEMENT)
            stratusZ = (float) - (5000 + 100 * (rand() % 150));
    }
    else
    {
        stratusZ = (float) - (100 * stratusBase + 100 * (rand() % 30));
    }

    stratus2Z = (float) - (100 * stratus2Base + 100 * (rand() % 10));
    stratusDepth = 1000.0f + 100 * (rand() % 30);
    stratus2Z = (float) - stratus2Base * 100.0f;

    ShadingFactor = 0;

    GenerateClouds();
}

void WeatherClass::UpdateCondition(int condition, bool bForce)
{
    weatherCondition = condition;

    if (weatherCondition not_eq oldWeatherCondition or bForce)
    {
        oldWeatherCondition = weatherCondition;
        needsWeatherRefresh = updateLighting = TRUE;

        switch (weatherCondition)
        {
            case SUNNY:
            {
                tempMin = 12;
                tempMed = 18;
                tempMax = 30;
                windMin = 0;
                windMed = 5;
                windMax = 10;

                wHdgThresh = 99;
                stratusBase = 220;
                stratus2Base = 350;
                stratusDepth = 2000.0f;
                contrailBase = 340;
                turbFactor = 0.1f;
                break;
            }

            case FAIR:
            {
                tempMin = 12;
                tempMed = 18;
                tempMax = 28;
                windMin = 5;
                windMed = 10;
                windMax = 20;

                wHdgThresh = 98;
                stratusBase = 220;
                stratus2Base = 350;
                stratusDepth = 2000.0f;
                cumulusBase = 80;
                contrailBase = 280;
                turbFactor = 0.2f;
                break;
            }

            case POOR:
            {
                tempMin = 10;
                tempMed = 15;
                tempMax = 25;
                windMin = 10;
                windMed = 15;
                windMax = 25;

                wHdgThresh = 97;
                stratusBase = 150;
                stratus2Base = 350;
                stratusDepth = 2000.0f;
                contrailBase = 250;
                turbFactor = 0.3f;
                break;
            }

            case INCLEMENT:
            {
                tempMin = 9;
                tempMed = 14;
                tempMax = 22;
                windMin = 15;
                windMed = 25;
                windMax = 35;

                wHdgThresh = 96;
                stratusBase = 100;
                stratus2Base = 350;
                stratusDepth = 3000.0f;
                contrailBase = 200;
                turbFactor = 0.4f;
            }
        }
    }
}

void WeatherClass::UpdateWeather()
{
    if ( not TheCampaign.IsMaster()) return;

    float seed, delta;
    CampaignTime time, tDelta;

    if (weatherDay not_eq TheCampaign.GetCurrentDay())
    {
        switch (TimeOfDayGeneral())
        {
            case TOD_NIGHT:
            {
                windSpeed = (float)(windMin + rand() % 5);
                temperature = (float)(tempMin + rand() % 3);
                break;
            }

            case TOD_DAWNDUSK:
            {
                windSpeed = (float)(windMin + rand() % 10);
                temperature = (float)(tempMin + rand() % 5);
                break;
            }

            default:
            {
                windSpeed = (float)(windMed + rand() % 20);
                temperature = (float)(min(tempMed + rand() % 10, tempMax));
            }
        }

        windHeading = (rand() % 360) * DTR;

        if (windHeading > 2.f * PI)
            windHeading -= (2.f * PI);

        contrailLow = 100.f * (float)(contrailBase + 100 * rand() % 10);
        contrailHigh = max(95000.f, (float)(2 + contrailLow + 1000 * (rand() % 8))); // FRB - For the SR-71
        //contrailHigh = max(35000.f, (float)(2+contrailLow+1000*rand()%8));

        cumulusZ = (float) - (100 * cumulusBase + 100 * rand() % 5);

        if (weatherCondition > FAIR)
            stratusZ = (float) - (100 * stratusBase + 100 * rand() % 5);
        else
            stratusZ = (float) - (100 * stratusBase + 100 * rand() % 10);

        stratus2Z = (float) - (100 * stratus2Base + 100 * rand() % 10);

        weatherDay = TheCampaign.GetCurrentDay();
    }

    time = Camp_GetCurrentTime();

    if (time - lastCheck > (CampaignMinutes / 2) or needsWeatherRefresh)
    {
        static int lastTOD = TOD_NIGHT;
        int h = FloatToInt32((windHeading - .5f * PI) * 3.f);
        FalconWeatherMessage *message = new FalconWeatherMessage(vuLocalSessionEntity->Id(), FalconLocalGame);

        tDelta = (CampaignTime)max(min((time - lastCheck), 2 * CampaignMinutes), 0.f);
        lastCheck = time;

        seed = (float)(rand() % 100) / 100.f;
        delta = (float)tDelta / (float)CampaignHours;

        switch (TimeOfDayGeneral())
        {
            case TOD_NIGHT:
            {
                if (temperature > tempMin)
                    temperature -= (float)delta * seed * 2.f;
                else
                    temperature += (float)delta * seed * 2.f;

                if (windSpeed > windMin)
                    windSpeed -= (float)delta * seed * 4.f;
                else
                    windSpeed += (float)delta * seed * 4.f;

                lastTOD = TOD_NIGHT;
                break;
            }

            case TOD_DAWNDUSK:
            {
                if (lastTOD == TOD_NIGHT)
                {
                    if (temperature < tempMed - 2)
                        temperature += (float)delta * seed * 4.f;
                    else
                        temperature -= (float)delta * seed * 4.f;

                    if (windSpeed < windMed - 5)
                        windSpeed += (float)delta * seed;
                    else
                        windSpeed -= (float)delta * seed;
                }
                else
                {
                    if (temperature > tempMin + 4)
                        temperature -= (float)delta * seed * 4.f;
                    else
                        temperature += (float)delta * seed * 4.f;

                    if (windSpeed > windMin + 5)
                        windSpeed -= (float)delta * seed * 4.f;
                    else
                        windSpeed += (float)delta * seed * 4.f;
                }

                break;
            }

            default:
            {
                if (temperature < tempMax)
                    temperature += (float)delta * seed * 2.f;
                else
                    temperature -= (float)delta * seed * 2.f;

                if (windSpeed < windMax)
                    windSpeed += (float)delta * seed * 4.f;
                else
                    windSpeed -= (float)delta * seed * 4.f;


                lastTOD = TOD_DAY;
            }
        }

        if (rand() % 100 > wHdgThresh or needsWeatherRefresh)
        {
            if (rand() % 8 > 4 + h)
                windHeading = windHeading + (float)delta / 3;
            else
                windHeading = windHeading - (float)delta / 3;

            if (windHeading < 0.f)
                windHeading += (float)(2.f * PI);

            if (windHeading > 2.f * PI)
                windHeading -= (float)(2.f * PI);
        }

        condCounter += rand() % 3;

        if (condCounter > 360)
        {
            condCounter = 0;
            int direction = rand() % 10;
            direction = (direction > 5) ? 1 : -1;

            switch (weatherCondition)
            {
                case SUNNY:
                {
                    UpdateCondition(weatherCondition + 1);
                    break;
                }

                case FAIR:
                {
                    UpdateCondition(weatherCondition + direction);
                    break;
                }

                case POOR:
                {
                    UpdateCondition(weatherCondition + direction);
                    break;
                }

                case INCLEMENT:
                {
                    UpdateCondition(weatherCondition - 1);
                }
            }
        }

        message->dataBlock.weatherCondition = weatherCondition;
        message->dataBlock.lastCheck = lastCheck;
        message->dataBlock.temperature = temperature;
        message->dataBlock.windSpeed = windSpeed;
        message->dataBlock.windHeading = windHeading;
        message->dataBlock.cumulusZ = cumulusZ;
        message->dataBlock.stratusZ = stratusZ;
        message->dataBlock.stratus2Z = stratus2Z;
        message->dataBlock.contrailLow = contrailLow;
        message->dataBlock.contrailHigh = contrailHigh;
        message->dataBlock.ShadingFactor = ShadingFactor;
        message->dataBlock.weatherQuality = WeatherQuality;

        FalconSendMessage(message, TRUE);
    }

    if (needsWeatherRefresh)
    {
        needsWeatherRefresh = FALSE;
    }


    // RED - Patch... as we use a single wind variable, at least do all needed calculations here for a vector
    mlTrig trigWind;
    mlSinCos(&trigWind, windHeading);
    WindVector.x = trigWind.cos * windSpeed;
    WindVector.y = trigWind.sin * windSpeed;
    WindVector.z = 0.0f;
}

void WeatherClass::SendWeather(VuTargetEntity *target)
{
    GenerateClouds(FALSE);

    FalconWeatherMessage *message;
    message = new FalconWeatherMessage(vuLocalSessionEntity->Id(), target);

    message->dataBlock.weatherCondition = weatherCondition;
    message->dataBlock.lastCheck = lastCheck;
    message->dataBlock.temperature = temperature;
    message->dataBlock.windSpeed = windSpeed;
    message->dataBlock.windHeading = windHeading;
    message->dataBlock.cumulusZ = cumulusZ;
    message->dataBlock.stratusZ = stratusZ;
    message->dataBlock.stratus2Z = stratus2Z;
    message->dataBlock.contrailLow = contrailLow;
    message->dataBlock.contrailHigh = contrailHigh;
    message->dataBlock.ShadingFactor = ShadingFactor;
    message->dataBlock.weatherQuality = WeatherQuality;

    FalconSendMessage(message, TRUE);
}

void WeatherClass::ReceiveWeather(FalconWeatherMessage* message)
{
    UpdateCondition(message->dataBlock.weatherCondition);
    lastCheck = message->dataBlock.lastCheck;
    temperature = message->dataBlock.temperature;
    windSpeed = message->dataBlock.windSpeed;
    windHeading = message->dataBlock.windHeading;
    cumulusZ = message->dataBlock.cumulusZ;
    stratusZ = message->dataBlock.stratusZ;
    stratus2Z = message->dataBlock.stratus2Z;
    contrailLow = message->dataBlock.contrailLow;
    contrailHigh = message->dataBlock.contrailHigh;
    ShadingFactor = message->dataBlock.ShadingFactor;
    WeatherQuality = message->dataBlock.weatherQuality;

    if (TheCampaign.Flags bitand CAMP_NEED_WEATHER)
    {
        GenerateClouds(FALSE);
        TheCampaign.Flags and_eq compl CAMP_NEED_WEATHER;
    }

    TheCampaign.GotJoinData();
}

int WeatherClass::CampLoad(char* name, int type)
{
    char /* *data,*/*data_ptr;
    BYTE utemp;
    float ftemp, ftemp1;

    Init((type == game_InstantAction or type == game_Dogfight));

    if (type == game_Campaign)
        unlockableCondition = TRUE;

    CampaignData cd = ReadCampFile(name, "wth");

    if (cd.dataSize == -1) return 0;

    data_ptr = cd.data;

    if (type not_eq game_Campaign and type not_eq game_PlayerPool)
    {
        if (gCampDataVersion >= 75)
        {
            UpdateCondition(*((int *)data_ptr));
            data_ptr += sizeof(int);

            lastCheck = *((CampaignTime *)data_ptr);
            data_ptr += sizeof(CampaignTime);

            temperature = *((float *)data_ptr);
            data_ptr += sizeof(float);

            windSpeed = *((float *)data_ptr);
            data_ptr += sizeof(float);

            windHeading = *((float *)data_ptr);
            data_ptr += sizeof(float);

            if (gCampDataVersion >= 76)
            {
                cumulusZ = *((float *)data_ptr);
                data_ptr += sizeof(float);
            }
            else
                cumulusZ = (float) - (100 * cumulusBase + 100 * rand() % 50);

            stratusZ = *((float *)data_ptr);
            data_ptr += sizeof(float);

            contrailLow = *((float *)data_ptr);
            data_ptr += sizeof(float);

            contrailHigh = *((float *)data_ptr);
            data_ptr += sizeof(float);
        }
        else
        {
            // Cobra - compatibility with Tacedit
            /* Tacedit label              Cobra Variables
            ====================     =========================
             Wind Direction         (4) Wind Heading (degrees)
             Wind Speed             (4) Cumulus Base (feet)
             Time                   (4) Campaign/TE Time
             Temperature            (4) Stratus Base (feet)
             Temp                   (1) Temperature (deg. Celcius)
             Wind                   (1) Wind Speed (knots)
             Cloud base             (1) Weather condition (1=Sunny,2=fair,3=poor,4=inclement)
             Con Layer Start        (1) Contrail Base (100's of feet)
             Con Layer End          (1) Overcast Depth (100's of feet)
             X Off                  (4) Stratus 2 Base (future)
             Y Off                  (4) Cobra file version (do not change) */

            // Cobra version check  (gCampDataVersion = 73 = SP3 version)
            // Tacedit reverses XOff and YOff when TE is saved. :^(
            if (*((float *)(data_ptr + 21)) == COVersion)
            {
                ftemp = *((float *)(data_ptr + 21));
                ftemp1 = *((float *)(data_ptr + 25));
            }
            else
            {
                ftemp = *((float *)(data_ptr + 25));
                ftemp1 = *((float *)(data_ptr + 21));
            }

            // if ((*((float *)(data_ptr+25))) == COVersion)
            if (ftemp == COVersion)
            {
                utemp = *((char *)(data_ptr + 18));
                UpdateCondition((int)utemp, false);

                windHeading = *((float *)data_ptr);
                data_ptr += sizeof(float);

                cumulusZ = *((float *)data_ptr);
                cumulusBase = (int)(cumulusZ / 100.f);
                cumulusZ = -cumulusZ;
                data_ptr += sizeof(float);

                lastCheck = *((CampaignTime *)data_ptr);
                data_ptr += sizeof(CampaignTime);

                stratusZ = *((float *)data_ptr);
                stratusBase = (int)(stratusZ / 100.f);
                stratusZ = -stratusZ;
                data_ptr += sizeof(float);

                utemp = *((char *)data_ptr);
                temperature = (float)(int)utemp;
                data_ptr += sizeof(char);

                utemp = *((char *)data_ptr);
                windSpeed = (float)utemp / (KPH_TO_FPS * FTPSEC_TO_KNOTS);
                data_ptr += sizeof(char);

                // utemp = *((char *)data_ptr);
                // UpdateCondition((int)utemp);
                data_ptr += sizeof(char);

                utemp = *((char *)data_ptr);
                contrailLow = (float)utemp * 1000.0f;
                contrailBase = (SLONG)(contrailLow / 100.f);
                contrailHigh = 95000.f;
                //contrailHigh = 45000.f;
                data_ptr += sizeof(char);

                utemp = *((char *)data_ptr);
                stratusDepth = (float)utemp * 100.0f;
                data_ptr += sizeof(char);

                stratus2Z = ftemp1;
                stratus2Base = (int)(stratus2Z / 100.f);
                stratus2Z = -stratus2Z;
            }
            else
            {
                // SUNNY=1  FAIR =2 POOR=3 INCLEMENT=4
                windHeading = *((float *) data_ptr);
                data_ptr += sizeof(float);
                windSpeed = *((float *) data_ptr);
                windSpeed = (float)windSpeed / (KPH_TO_FPS * FTPSEC_TO_KNOTS);
                data_ptr += sizeof(float);
                lastCheck = *((CampaignTime *)
                              data_ptr);
                data_ptr += sizeof(CampaignTime);
                temperature = *((float *) data_ptr);
                data_ptr += sizeof(float);
                // TodaysTemp = *((uchar *) data_ptr);
                data_ptr += sizeof(uchar);
                // TodaysWind = *((uchar *) data_ptr);
                data_ptr += sizeof(uchar);
                cumulusBase = (int) * ((uchar *) data_ptr);

                if (cumulusBase < 100)
                    cumulusBase = 100;

                cumulusZ = -(float)cumulusBase * 100.f;
                data_ptr += sizeof(uchar);
                contrailLow = (float) * ((uchar *) data_ptr);
                contrailLow *= 1000.0f;
                contrailBase = (SLONG)(contrailLow / 100.0f);
                data_ptr += sizeof(uchar);
                contrailHigh = (float) * ((uchar *) data_ptr);
                contrailHigh *= 1000.0f;

                if (contrailHigh < 95000.f)
                    contrailHigh = 95000.f;

                //if (contrailHigh < 45000.f)
                // contrailHigh = 45000.f;
                stratusBase = 220;
                stratus2Base = 350;
                stratusZ = -22000.f;
                stratus2Z = -35000.f;

                if (PlayerOptions.weatherCondition < 1 or PlayerOptions.weatherCondition > 4)
                    PlayerOptions.weatherCondition = 1;

                UpdateCondition(PlayerOptions.weatherCondition, false);
                UpdateWeather();
            }
        }
    }

    delete cd.data;

    // RED - Update the weather condition
    realWeather->UpdateCondition();

    return TRUE;
}

int WeatherClass::Save(char* name)
{
    FILE *fp;
    UINT nw = 0, nh = 0;
    unsigned int w = 0, h = 0;

    if ((fp = OpenCampFile(name, "wth", "wb")) == NULL) return 0;

    if (gCurrentDataVersion >= 75)
    {
        fwrite(&weatherCondition, sizeof(int), 1, fp);
        fwrite(&lastCheck, sizeof(CampaignTime), 1, fp);
        fwrite(&temperature, sizeof(float), 1, fp);
        fwrite(&windSpeed, sizeof(float), 1, fp);
        fwrite(&windHeading, sizeof(float), 1, fp);
        fwrite(&cumulusZ, sizeof(float), 1, fp);
        fwrite(&stratusZ, sizeof(float), 1, fp);
        fwrite(&contrailLow, sizeof(float), 1, fp);
        fwrite(&contrailHigh, sizeof(float), 1, fp);
    }
    // Cobra - compatibility with Tacedit
    /* Tacedit label              Cobra Variables
    ====================     =========================
      Wind Direction         (4) Wind Heading (degrees)
      Wind Speed             (4) Cumulus Base (feet)
      Time                   (4) Campaign/TE Time
      Temperature            (4) Stratus Base (feet)
      Temp                   (1) Temperature (deg. Celcius)
      Wind                   (1) Wind Speed (knots)
      Cloud base             (1) Weather condition (1=Sunny,2=fair,3=poor,4=inclement)
      Con Layer Start        (1) Contrail Base (100's of feet)
      Con Layer End          (1) Overcast Depth (100's of feet)
      X Off                  (4) Stratus 2 Base (future)
      Y Off                  (4) Cobra file version (do not change) */

    else
    {
        UINT uix = 0;
        BYTE uConv;
        char sConv;
        float fTemp;

        fwrite(&windHeading, sizeof(float), 1, fp);
        fTemp = -cumulusZ; // WindSpeed
        fwrite(&fTemp, sizeof(float), 1, fp);
        fwrite(&lastCheck, sizeof(CampaignTime), 1, fp);
        fTemp = -stratusZ; // Temperature
        fwrite(&fTemp, sizeof(float), 1, fp);
        sConv = (char)(int)temperature; // TodaysTemp
        fwrite(&sConv, sizeof(char), 1, fp);
        uConv = (BYTE)(windSpeed * (KPH_TO_FPS * FTPSEC_TO_KNOTS));
        fwrite(&uConv, sizeof(BYTE), 1, fp); // TodaysWind
        uConv = (BYTE)weatherCondition;
        fwrite(&uConv, sizeof(BYTE), 1, fp); // TodaysBase
        uConv = (BYTE)(int)(contrailLow / 1000.0f + 0.5); // same
        fwrite(&uConv, sizeof(BYTE), 1, fp);
        uConv = (BYTE)(int)(stratusDepth / 100.0f); // TodaysConHigh
        fwrite(&uConv, sizeof(BYTE), 1, fp);
        fTemp = -stratus2Z; // Temperature
        fwrite(&fTemp, sizeof(float), 1, fp); // offsetX
        fwrite(&COVersion, sizeof(float), 1, fp); // offsetY
        fwrite(&uix, sizeof(UINT), 1, fp); // map width
        fwrite(&uix, sizeof(UINT), 1, fp); // map height
        // fwrite(map,sizeof(CellState),w*h,fp);
    }

    CloseCampFile(fp);
    return 1;
}

float WeatherClass::TemperatureAt(const Tpoint *pos)
{
    float alt = -pos->z / 1000;
    return temperature - 3 * alt;
}

float WeatherClass::WindSpeedInFeetPerSecond(const Tpoint *pos)
{
    return windSpeed * 0.9113f;
}

float WeatherClass::WindHeadingAt(const Tpoint *pos)
{
    return windHeading;
}

//FIXME
int WeatherClass::GetCloudCover(GridIndex x, GridIndex y)
{
    return 0;
}

//FIXME
int WeatherClass::GetCloudLevel(GridIndex x, GridIndex y)
{
    return 0;
}

//FIXME
void WeatherClass::SetCloudCover(GridIndex x, GridIndex y, int cov)
{
}

//FIXME
void WeatherClass::SetCloudLevel(GridIndex x, GridIndex y, int lev)
{
}
