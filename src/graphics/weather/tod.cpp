#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "grmath.h"
#include "grinline.h"
#include "PalBank.h"
#include "TimeMgr.h"
#include "RViewpnt.h"
#include "RenderOW.h"
#include "tod.h"
#include "Falclib/include/PlayerOp.h"
#include "Star.h"
#include "RealWeather.h"
#include <fstream>
#include <iostream>

using namespace std;

CStar TheStar;

// The one and only time manager
CTimeOfDay TheTimeOfDay;

extern float g_fLatitude; // JB 010804

// but this compiled, strange.
SkyColorDataType* skycolor;

// size of sun glare area = 22.5 deg
static const int SUN_GLARE_SIZE = 1024;

unsigned char CTimeOfDay::MoonPhaseMask[8 * 64];
unsigned char CTimeOfDay::CurrentMoonPhaseMask[8 * 64];

void CTimeOfDay::Setup(char *dataPath)
{
    char todfile[_MAX_PATH];
    char starfile[_MAX_PATH];
    FILE *in;

    ShiAssert( not IsReady());

    // Construct the input filename we need
    if (skycolor)
        sprintf(todfile, "%s\\tod\\%s", dataPath, skycolor[PlayerOptions.skycol - 1].todname);

    sprintf(starfile, "%s\\star.dat", dataPath);

    if ( not skycolor or not (in = fopen(todfile, "r"))) // Oops, the todfile is not there ? Use default one
    {
        sprintf(todfile, "%s\\tod\\tod.lst.default", dataPath);

        if ( not (in = fopen(todfile, "r"))) // Oops, the todfile is not there ? Use default one
            sprintf(todfile, "%s\\tod.lst", dataPath);
        else
            fclose(in);
    }
    else
        fclose(in);

    sprintf(todfile, "%s\\tod.lst", dataPath);
    in = fopen(todfile, "r");

    if (in == NULL)
    {
        char string[256];
        sprintf(string, "TOD file open failed:  %s", todfile);
        ShiError(string);
        // We need to exit gracefully
        return;
    }

    TimeOfDayStruct temptod;
    TotalTimeOfDay = ReadTODFile(in, &temptod, 1);

    if ( not TotalTimeOfDay)
    {
        fclose(in);
        char string[256];
        sprintf(string, "No data obtained from TOD file:  %s", todfile);
        ShiError(string);
    }

    TimeOfDay = new TimeOfDayStruct[TotalTimeOfDay];

    if ( not TimeOfDay)
    {
        fclose(in);
        ShiError("Failed TOD memory allocation");
    }

    fseek(in, 0, 0);

    ISunYaw = IMoonYaw = 4096;
    ISunTilt = IMoonTilt = 0;
    HazeSunriseColor.r = 1.0f;
    HazeSunriseColor.g = 0.6f;
    HazeSunriseColor.b = 0.1f;
    HazeSunsetColor.r = 1.0f;
    HazeSunsetColor.g = 0.6f;
    HazeSunsetColor.b = 0.1f;
    Flag = 0;

    ReadTODFile(in, TimeOfDay - 1);
    fclose(in);

    SetVar(TimeOfDay);

    ////////////////
    if (TheStar.Setup(starfile, 11.0f))   // load all stars with magnitude less than 11
    {
        //ShiError ("Failed Loading Star");
        // We nead to exit cleanly?
    }

    TheStarData = TheStar.GetStarData();
    TheStar.SetHorizon((float) degtorad(5), (float) degtorad(15)); // display stars with elevation > horizon
    // M.N. changed back from theater.map readout. It seems that the sun position is normalized to korean latitude,
    // so no need to change it at all...
    TheStar.SetLocation(g_fLatitude, 0.0f); // latitude, longitude

    /*
     struct tm *newtime;
     long ltime;
     time( &ltime );
     newtime = gmtime( &ltime );
     TheStar.SetDate (newtime -> tm_mday, newtime -> tm_mon + 1, newtime -> tm_year + 1900);
    */
    int year = TheTimeManager.GetYearAD();
    int extraday = TheTimeManager.GetDayOfYear();
    int month = 1;
    int day = 0;
    TheStar.CalculateDate(&day, &month, &year, extraday);
    TheStar.SetDate(day, month, year);

    lastMoonTime = 0;
    MoonPhase = -1;
    ////////////////////

    int i, j;

    for (i = 0; i < TotalTimeOfDay; i++)
    {
        j = i + 1;

        if (j >= TotalTimeOfDay) j = 0;

        if ( not (TimeOfDay[j].Flag bitand GL_TIME_OF_DAY_USE_SUN))
            TimeOfDay[i].Flag and_eq compl GL_TIME_OF_DAY_USE_SUN;

        if ( not (TimeOfDay[j].Flag bitand GL_TIME_OF_DAY_USE_MOON))
            TimeOfDay[i].Flag and_eq compl GL_TIME_OF_DAY_USE_MOON;

        int k = 0;

        if (TimeOfDay[i].StarIntensity > 0.0f) k = 1;
        else if (TimeOfDay[j].StarIntensity > 0.0f) k = 1;

        if (k) TimeOfDay[i].Flag or_eq GL_TIME_OF_DAY_USE_STAR;
    }


    // Default lighting conditions
    Ambient = .3f;
    Diffuse = .6f;
    Specular = .8f;

    // default glare angle is 22.5 deg
    SetSunGlareAngle(1024);

    // Start with NVGs off
    NVGmode = FALSE;

    // Initialize the lighting conditions and register for future time of day updates
    TimeUpdateCallback(this);
    TheTimeManager.RegisterTimeUpdateCB(TimeUpdateCallback, this);
}

void CTimeOfDay::Cleanup()
{
    ShiAssert(IsReady());

    // Stop receiving time updates
    TheTimeManager.ReleaseTimeUpdateCB(TimeUpdateCallback, this);

    // Clean up and release the start position data
    TheStar.Cleanup();

    // Clean up and release the time of data transition data
    delete[] TimeOfDay;
    TimeOfDay = NULL;
}

void CTimeOfDay::SetNVGmode(BOOL state)
{
    NVGmode = state;
    TheTimeManager.Refresh();
}

// Update the sky colors and sun/moon position based on the current time of day
void CTimeOfDay::TimeUpdateCallback(void *self)
{
    ((CTimeOfDay*)self)->UpdateSkyProperties();
}

void CTimeOfDay::UpdateSkyProperties()
{
    int i, c, n;
    TimeOfDayStruct *tod, *ntod;
    unsigned now;
    float t;

    // Convert from time since clock start to time since midnight
    now = TheTimeManager.GetTimeOfDay();

    unsigned curtime = TheTimeManager.GetClockTime();
    TheStar.SetUniversalTime(curtime);

    if (curtime)
    {
        if (lastMoonTime == 0 or lastMoonTime > curtime or ((curtime - lastMoonTime) > 60 * 60 * 1000))
        {
            lastMoonTime = curtime;
            MoonPhase = -1;
            MoonPhase = CalculateMoonPercent();
            CreateMoonPhaseMask(MoonPhaseMask, MoonPhase);
        }
    }

    // Identify the Current time step in the TOD table
    for (i = 0; i < TotalTimeOfDay; i++)
        if (TimeOfDay[i].Time > now)
            break;

    if (i == 0)
        c = TotalTimeOfDay - 1;
    else if (i >= TotalTimeOfDay)
        c = TotalTimeOfDay - 1;
    else c = i - 1;

    // Identify the Next time step in the TOD table
    n = c + 1;

    if (n >= TotalTimeOfDay)
        n = 0;

    // This should only happen if the table has less than two entries
    if (c == n)
        return;

    // Get pointers to the current and next TOD table entries
    tod = &(TimeOfDay[c]);
    ntod = &(TimeOfDay[n]);

    // No two table entries should have the same time stamp
    c = tod -> Time;
    n = ntod -> Time;
    ShiAssert(c not_eq n);

    // Calculate the time between the two table entries
    if (n < c)
        n += MSEC_PER_DAY;

    n -= c;

    // Calculate the time between now and the current table entry
    if (now < (DWORD)c)
        now += MSEC_PER_DAY;

    c = now - c;

    // Calculate the interpolation control variable "t"
    t = (float) c / (float) n;

    // Set all our variable from the first record
    SetVar(tod);

    // Add in deltas toward the second record
    m_SkyColor.r += t * (ntod -> SkyColor.r - m_SkyColor.r);
    m_SkyColor.g += t * (ntod -> SkyColor.g - m_SkyColor.g);
    m_SkyColor.b += t * (ntod -> SkyColor.b - m_SkyColor.b);
    m_HazeSkyColor.r += t * (ntod -> HazeSkyColor.r - m_HazeSkyColor.r);
    m_HazeSkyColor.g += t * (ntod -> HazeSkyColor.g - m_HazeSkyColor.g);
    m_HazeSkyColor.b += t * (ntod -> HazeSkyColor.b - m_HazeSkyColor.b);
    m_GroundColor.r += t * (ntod -> GroundColor.r - m_GroundColor.r);
    m_GroundColor.g += t * (ntod -> GroundColor.g - m_GroundColor.g);
    m_GroundColor.b += t * (ntod -> GroundColor.b - m_GroundColor.b);
    m_HazeGroundColor.r += t * (ntod -> HazeGroundColor.r - m_HazeGroundColor.r);
    m_HazeGroundColor.g += t * (ntod -> HazeGroundColor.g - m_HazeGroundColor.g);
    m_HazeGroundColor.b += t * (ntod -> HazeGroundColor.b - m_HazeGroundColor.b);
    m_TextureLighting.r += t * (ntod -> TextureLighting.r - m_TextureLighting.r);
    m_TextureLighting.g += t * (ntod -> TextureLighting.g - m_TextureLighting.g);
    m_TextureLighting.b += t * (ntod -> TextureLighting.b - m_TextureLighting.b);

    m_BadWeatherLighting.r += t * (ntod -> BadWeatherLighting.r - m_BadWeatherLighting.r);
    m_BadWeatherLighting.g += t * (ntod -> BadWeatherLighting.g - m_BadWeatherLighting.g);
    m_BadWeatherLighting.b += t * (ntod -> BadWeatherLighting.b - m_BadWeatherLighting.b);

    m_VisColor.r += t * (ntod -> VisColor.r - m_VisColor.r);
    m_VisColor.g += t * (ntod -> VisColor.g - m_VisColor.g);
    m_VisColor.b += t * (ntod -> VisColor.b - m_VisColor.b);

    Tcolor Color;
    Color = tod->RainColor;
    Color.r += t * (ntod -> RainColor.r - tod->RainColor.r);
    Color.g += t * (ntod -> RainColor.g - tod->RainColor.g);
    Color.b += t * (ntod -> RainColor.b - tod->RainColor.b);
    RainColor = MakeColor(&Color);

    Color = tod->SnowColor;
    Color.r += t * (ntod -> SnowColor.r - tod->SnowColor.r);
    Color.g += t * (ntod -> SnowColor.g - tod->SnowColor.g);
    Color.b += t * (ntod -> SnowColor.b - tod->SnowColor.b);
    SnowColor = MakeColor(&Color);

    LightningColor.r += t * (ntod -> LightningColor.r - tod->LightningColor.r);
    LightningColor.g += t * (ntod -> LightningColor.g - tod->LightningColor.g);
    LightningColor.b += t * (ntod -> LightningColor.b - tod->LightningColor.b);

    m_Ambient += t * (ntod -> Ambient - m_Ambient);
    m_Diffuse += t * (ntod -> Diffuse - m_Diffuse);
    m_Specular += t * (ntod -> Specular - m_Specular);
    m_MinVis += t * (ntod -> MinVis  - m_MinVis);

    float ra, dec, az, alt;
    TheStar.GetSunRaDec(&ra, &dec);
    TheStar.ConvertPosition(ra, dec, &az, &alt);
    TheStar.SetSunPosition(az, alt);
    TheStar.ConvertCoord(ra, dec, &SunCoord.x, &SunCoord.y, &SunCoord.z);
    ISunYaw = FloatToInt32(radtoangle(az));
    ISunPitch = FloatToInt32(radtoangle(alt));
    TheStar.GetMoonRaDec(&ra, &dec);
    TheStar.ConvertPosition(ra, dec, &az, &alt);
    TheStar.SetMoonPosition(az, alt);
    TheStar.ConvertCoord(ra, dec, &MoonCoord.x, &MoonCoord.y, &MoonCoord.z);
    IMoonYaw = FloatToInt32(radtoangle(az));
    IMoonPitch = FloatToInt32(radtoangle(alt));

    if (ISunPitch < 256 or ISunPitch > (8192 - 256))
    {
        // Adjust the light level for the moon
        // (original levels are assumed to have been for a full moon)
        // At new moon and/or moon rise/set, we darken by at most 1/2
        // (at little more, actually, since the SIN can become negative just as the moon sets/rises)
        float t1 = (float) abs(NEW_MOON_PHASE - CalculateMoonPercent());
        t1 = (t1 / NEW_MOON_PHASE) * (float)sin(alt);//angletorad(IMoonPitch));
        t1 = (1.0f + t1) / 2.0f;

        if (t1 < 0.45f) t1 = 0.45f; // limit the darkness level

        m_HazeGroundColor.r *= t1;
        m_HazeGroundColor.g *= t1;
        m_HazeGroundColor.b *= t1;
        m_TextureLighting.r *= t1;
        m_TextureLighting.g *= t1;
        m_TextureLighting.b *= t1;

        m_BadWeatherLighting.r *= t1;
        m_BadWeatherLighting.g *= t1;
        m_BadWeatherLighting.b *= t1;

        m_Ambient *= t1;
        m_Diffuse *= t1;
        m_Specular *= t1;
    }

    // Update the positions and effects of the celstial objects
    m_StarIntensity += t * (ntod -> StarIntensity - m_StarIntensity);
    TheStar.UpdateStar();
    /*
     if(realWeather->weatherCondition > FAIR)
     {
     if(realWeather->weatherCondition == INCLEMENT)
     {
     BadWeatherLighting.r = max(BadWeatherLighting.r/1.5f,0.01f);
     BadWeatherLighting.g = max(BadWeatherLighting.g/1.5f,0.01f);
     BadWeatherLighting.b = max(BadWeatherLighting.b/1.5f,0.01f);
     }

     if(realWeather->InsideOvercast() or realWeather->UnderOvercast())
     {
     if(realWeather->weatherCondition > POOR)
      Specular = 0.f;
     else
      Specular *= 0.2f;

     SkyColor.r = BadWeatherLighting.r/max((1.25f*scaleFactor),1.f);
     SkyColor.g = BadWeatherLighting.g/max((1.25f*scaleFactor),1.f);
     SkyColor.b = BadWeatherLighting.b/max((1.25f*scaleFactor),1.f);

     HazeSkyColor.r = BadWeatherLighting.r/max((1.25f*scaleFactor),1.f);
     HazeSkyColor.g = BadWeatherLighting.g/max((1.25f*scaleFactor),1.f);
     HazeSkyColor.b = BadWeatherLighting.b/max((1.25f*scaleFactor),1.f);

     HazeGroundColor.r = BadWeatherLighting.r/max((1.25f*scaleFactor),1.f);
     HazeGroundColor.g = BadWeatherLighting.g/max((1.25f*scaleFactor),1.f);
     HazeGroundColor.b = BadWeatherLighting.b/max((1.25f*scaleFactor),1.f);

     GroundColor.r = BadWeatherLighting.r/max((1.25f*scaleFactor),1.f);
     GroundColor.g = BadWeatherLighting.g/max((1.25f*scaleFactor),1.f);
     GroundColor.b = BadWeatherLighting.b/max((1.25f*scaleFactor),1.f);

     TextureLighting.r = BadWeatherLighting.r/max((1.25f*scaleFactor),1.f);
     TextureLighting.g = BadWeatherLighting.g/max((1.25f*scaleFactor),1.f);
     TextureLighting.b = BadWeatherLighting.b/max((1.25f*scaleFactor),1.f);
     }
     else
     {
     SkyColor.r = (SkyColor.r*(1.f - scaleFactor))+(BadWeatherLighting.r*scaleFactor);
     SkyColor.g = (SkyColor.g*(1.f - scaleFactor))+(BadWeatherLighting.g*scaleFactor);
     SkyColor.b = (SkyColor.b*(1.f - scaleFactor))+(BadWeatherLighting.b*scaleFactor);

     HazeSkyColor.r = min(BadWeatherLighting.r/max((1.25f*scaleFactor),.67f),.9f);
     HazeSkyColor.g = min(BadWeatherLighting.g/max((1.25f*scaleFactor),.67f),.9f);
     HazeSkyColor.b = min(BadWeatherLighting.b/max((1.25f*scaleFactor),.67f),.9f);

     HazeGroundColor.r = min(BadWeatherLighting.r/max((1.25f*scaleFactor),.67f),.9f);
     HazeGroundColor.g = min(BadWeatherLighting.g/max((1.25f*scaleFactor),.67f),.9f);
     HazeGroundColor.b = min(BadWeatherLighting.b/max((1.25f*scaleFactor),.67f),.9f);

     GroundColor.r = min(BadWeatherLighting.r/max((1.25f*scaleFactor),.67f),.9f);
     GroundColor.g = min(BadWeatherLighting.g/max((1.25f*scaleFactor),.67f),.9f);
     GroundColor.b = min(BadWeatherLighting.b/max((1.25f*scaleFactor),.67f),.9f);

     TextureLighting.r = min(BadWeatherLighting.r/max((1.25f*scaleFactor),.67f),.9f);
     TextureLighting.g = min(BadWeatherLighting.g/max((1.25f*scaleFactor),.67f),.9f);
     TextureLighting.b = min(BadWeatherLighting.b/max((1.25f*scaleFactor),.67f),.9f);
     }

     Diffuse /= max((2.f*scaleFactor),1.f);
     }*/
}


// RED - Tihs function calculates Sky colors based on weather Conditions
void CTimeOfDay::UpdateWeatherColors(DWORD weatherCondition)
{
    // default values
    StarIntensity = m_StarIntensity;
    MinVis = m_MinVis;
    VisColor = m_VisColor;
    Ambient = m_Ambient;
    Specular = m_Specular;

    // Deafule values if nont under/inside an overcast layer
    if (weatherCondition <= FAIR)
    {
        SkyColor = m_SkyColor;
        HazeSkyColor = m_HazeSkyColor;
        GroundColor = m_GroundColor;
        HazeGroundColor = m_HazeGroundColor;
        TextureLighting = m_TextureLighting;
        BadWeatherLighting = m_BadWeatherLighting;
        Diffuse = m_Diffuse;
    }
    else
    {

        Diffuse = m_Diffuse / max((2.f * scaleFactor), 1.f);

        // Bad weather stuff
        if (weatherCondition == INCLEMENT)
        {
            BadWeatherLighting.r = max(m_BadWeatherLighting.r / 1.5f, 0.01f);
            BadWeatherLighting.g = max(m_BadWeatherLighting.g / 1.5f, 0.01f);
            BadWeatherLighting.b = max(m_BadWeatherLighting.b / 1.5f, 0.01f);
        }
        else
        {
            BadWeatherLighting.r = m_BadWeatherLighting.r;
            BadWeatherLighting.g = m_BadWeatherLighting.g;
            BadWeatherLighting.b = m_BadWeatherLighting.b;
        }

        if (realWeather->InsideOvercast() or realWeather->UnderOvercast())
        {

            if (realWeather->weatherCondition > POOR) Specular = 0.f;
            else Specular *= 0.2f;

            SkyColor.r = BadWeatherLighting.r / max((1.25f * scaleFactor), 1.f);
            SkyColor.g = BadWeatherLighting.g / max((1.25f * scaleFactor), 1.f);
            SkyColor.b = BadWeatherLighting.b / max((1.25f * scaleFactor), 1.f);

            HazeSkyColor.r = BadWeatherLighting.r / max((1.25f * scaleFactor), 1.f);
            HazeSkyColor.g = BadWeatherLighting.g / max((1.25f * scaleFactor), 1.f);
            HazeSkyColor.b = BadWeatherLighting.b / max((1.25f * scaleFactor), 1.f);

            HazeGroundColor.r = BadWeatherLighting.r / max((1.25f * scaleFactor), 1.f);
            HazeGroundColor.g = BadWeatherLighting.g / max((1.25f * scaleFactor), 1.f);
            HazeGroundColor.b = BadWeatherLighting.b / max((1.25f * scaleFactor), 1.f);

            GroundColor.r = BadWeatherLighting.r / max((1.25f * scaleFactor), 1.f);
            GroundColor.g = BadWeatherLighting.g / max((1.25f * scaleFactor), 1.f);
            GroundColor.b = BadWeatherLighting.b / max((1.25f * scaleFactor), 1.f);

            TextureLighting.r = BadWeatherLighting.r / max((1.25f * scaleFactor), 1.f);
            TextureLighting.g = BadWeatherLighting.g / max((1.25f * scaleFactor), 1.f);
            TextureLighting.b = BadWeatherLighting.b / max((1.25f * scaleFactor), 1.f);
        }
        else
        {
            SkyColor.r = (m_SkyColor.r * (1.f - scaleFactor)) + (BadWeatherLighting.r * scaleFactor);
            SkyColor.g = (m_SkyColor.g * (1.f - scaleFactor)) + (BadWeatherLighting.g * scaleFactor);
            SkyColor.b = (m_SkyColor.b * (1.f - scaleFactor)) + (BadWeatherLighting.b * scaleFactor);

            HazeSkyColor.r = min(BadWeatherLighting.r / max((1.25f * scaleFactor), .67f), .9f);
            HazeSkyColor.g = min(BadWeatherLighting.g / max((1.25f * scaleFactor), .67f), .9f);
            HazeSkyColor.b = min(BadWeatherLighting.b / max((1.25f * scaleFactor), .67f), .9f);

            HazeGroundColor.r = min(BadWeatherLighting.r / max((1.25f * scaleFactor), .67f), .9f);
            HazeGroundColor.g = min(BadWeatherLighting.g / max((1.25f * scaleFactor), .67f), .9f);
            HazeGroundColor.b = min(BadWeatherLighting.b / max((1.25f * scaleFactor), .67f), .9f);

            GroundColor.r = min(BadWeatherLighting.r / max((1.25f * scaleFactor), .67f), .9f);
            GroundColor.g = min(BadWeatherLighting.g / max((1.25f * scaleFactor), .67f), .9f);
            GroundColor.b = min(BadWeatherLighting.b / max((1.25f * scaleFactor), .67f), .9f);

            TextureLighting.r = min(BadWeatherLighting.r / max((1.25f * scaleFactor), .67f), .9f);
            TextureLighting.g = min(BadWeatherLighting.g / max((1.25f * scaleFactor), .67f), .9f);
            TextureLighting.b = min(BadWeatherLighting.b / max((1.25f * scaleFactor), .67f), .9f);
        }

    }
}




void CTimeOfDay::CalculateSunGroundPos(Tpoint *pos)
{
    pos -> x = SunCoord.x;
    pos -> y = SunCoord.y;
    pos -> z = 0.0f;
}

// Return point on unit sphere at center of sun/moon
void CTimeOfDay::CalculateSunMoonPos(Tpoint *pos, int ismoon)
{
    if (ismoon) *pos = MoonCoord;
    else *pos = SunCoord;
}


// Return the unit vector TOWARD the light source
// (Remember, X is north, Y is east, and Z is down)
void CTimeOfDay::GetLightDirection(Tpoint *LightDirection)
{
    float sH, cH, sC, cC;

    ShiAssert(LightDirection);

    // See if the sun is up
    if (ThereIsASun())
    {
        glGetSinCos(&sH, &cH, ISunYaw);
        glGetSinCos(&sC, &cC, ISunPitch);
    }
    else if (ThereIsAMoon())
    {
        glGetSinCos(&sH, &cH, IMoonYaw);
        glGetSinCos(&sC, &cC, IMoonPitch);
    }
    else
    {
        cC = 0.0f;
        sC = 1.0f;
        cH = 0.0F;
        sH = 0.0F;
    }

    LightDirection->x = cC * cH;
    LightDirection->y = cC * sH;
    LightDirection->z = -sC;
}


void CTimeOfDay::SetVar(TimeOfDayStruct *tod)
{
    m_SkyColor = tod ->SkyColor;
    m_HazeSkyColor = tod ->HazeSkyColor;
    m_GroundColor = tod ->GroundColor;
    m_HazeGroundColor = tod ->HazeGroundColor;
    m_TextureLighting = tod ->TextureLighting;
    m_BadWeatherLighting = tod ->BadWeatherLighting;
    m_Ambient = tod ->Ambient;
    m_Diffuse = tod ->Diffuse;
    m_Specular = tod ->Specular;
    Flag = tod -> Flag;
    m_StarIntensity = tod -> StarIntensity;
    RainColor = MakeColor(&tod->RainColor);
    SnowColor = MakeColor(&tod->SnowColor);
    LightningColor = tod->LightningColor;
    m_MinVis = tod->MinVis;
    m_VisColor = tod->VisColor;
}

void CTimeOfDay::SetDefaultColor(Tcolor *col, Tcolor *defcol)
{
    if (col->r == -1)
        *col = *defcol;
}

int CTimeOfDay::ReadTODFile(FILE *in, TimeOfDayStruct *tod, int countflag)
{
    float fvar;
    int total;
    char buffer[80] = { '\0' };

    total = 0;

    while (true)
    {
        fscanf(in, "%s", buffer);

        if (feof(in))
        {
            SetDefaultColor(&tod->RainColor, &tod->HazeGroundColor);
            SetDefaultColor(&tod->SnowColor, &tod->HazeGroundColor);
            SetDefaultColor(&tod->VisColor, &tod->HazeSkyColor);

            break;
        }
        else if (stricmp(buffer, "Time") == 0)
        {
            DWORD ivar1, ivar2, ivar3;

            if (total not_eq 0)
            {
                SetDefaultColor(&tod->RainColor, &tod->HazeGroundColor);
                SetDefaultColor(&tod->SnowColor, &tod->HazeGroundColor);
                SetDefaultColor(&tod->VisColor, &tod->HazeSkyColor);
            }

            ++total;

            if ( not countflag)
                ++tod;

            fscanf(in, "%ld:%ld:%ld", &ivar1, &ivar2, &ivar3);
            ivar1 *= 3600000;
            ivar2 *= 60000;
            ivar3 *= 1000;
            tod->Time = ivar1 + ivar2 + ivar3;
            tod->Flag = 0;
            tod->StarIntensity = 0.0f;
            tod->LightningColor.r = tod->LightningColor.g = 1;
            tod->LightningColor.b = 0;
            tod->RainColor.r = tod->RainColor.g = tod->RainColor.b = 1;
            tod->SnowColor.r = tod->SnowColor.g = tod->SnowColor.b = 1;
            tod->VisColor.r = tod->VisColor.g = tod->VisColor.b = -1;
            tod->MinVis = 0.1f;
        }
        else if (stricmp(buffer, "SunTilt") == 0)
        {
            fscanf(in, "%f", &fvar);
            ISunTilt = glConvertFromDegree(fvar);
        }
        else if (stricmp(buffer, "MoonTilt") == 0)
        {
            fscanf(in, "%f", &fvar);
            IMoonTilt = glConvertFromDegree(fvar);
        }
        else if (stricmp(buffer, "HazeSunsetColor") == 0)
            fscanf(in, "%f %f %f", &HazeSunsetColor.r, &HazeSunsetColor.g, &HazeSunsetColor.b);
        else if (stricmp(buffer, "HazeSunriseColor") == 0)
            fscanf(in, "%f %f %f", &HazeSunriseColor.r, &HazeSunriseColor.g, &HazeSunriseColor.b);
        else if (stricmp(buffer, "SkyColor") == 0)
            fscanf(in, "%f %f %f", &tod->SkyColor.r, &tod->SkyColor.g, &tod->SkyColor.b);
        else if (stricmp(buffer, "HazeSkyColor") == 0)
            fscanf(in, "%f %f %f", &tod->HazeSkyColor.r, &tod->HazeSkyColor.g, &tod->HazeSkyColor.b);
        else if (stricmp(buffer, "GroundColor") == 0)
            fscanf(in, "%f %f %f", &tod->GroundColor.r, &tod->GroundColor.g, &tod->GroundColor.b);
        else if (stricmp(buffer, "HazeGroundColor") == 0)
        {
            fscanf(in, "%f %f %f", &tod->HazeGroundColor.r, &tod->HazeGroundColor.g, &tod->HazeGroundColor.b);
            tod->HazeGroundColor.r *= 0.7f;
            tod->HazeGroundColor.g *= 0.7f;
            tod->HazeGroundColor.b *= 0.7f;
        }
        else if (stricmp(buffer, "TextureLighting") == 0)
            fscanf(in, "%f %f %f", &tod->TextureLighting.r, &tod->TextureLighting.g, &tod->TextureLighting.b);
        else if (stricmp(buffer, "BadWeatherLighting") == 0)
            fscanf(in, "%f %f %f", &tod->BadWeatherLighting.r, &tod->BadWeatherLighting.g, &tod->BadWeatherLighting.b);
        else if (stricmp(buffer, "Ambient") == 0)
            fscanf(in, "%f", &tod->Ambient);
        else if (stricmp(buffer, "Diffuse") == 0)
            fscanf(in, "%f", &tod->Diffuse);
        else if (stricmp(buffer, "Specular") == 0)
            fscanf(in, "%f", &tod->Specular);
        else if (stricmp(buffer, "SunPitch") == 0)
        {
            fscanf(in, "%f", &tod->SunPitch);
            tod->SunPitch = glConvertFromDegreef(tod->SunPitch);
            tod->Flag or_eq GL_TIME_OF_DAY_USE_SUN;
        }
        else if (stricmp(buffer, "MoonPitch") == 0)
        {
            fscanf(in, "%f", &tod->MoonPitch);
            tod->MoonPitch = glConvertFromDegreef(tod->MoonPitch);
            tod->Flag or_eq GL_TIME_OF_DAY_USE_MOON;
        }
        else if (stricmp(buffer, "Star") == 0)
            tod->StarIntensity = 1.0f;
        else if (stricmp(buffer, "RainColor") == 0)
            fscanf(in, "%f %f %f", &tod->RainColor.r, &tod->RainColor.g, &tod->RainColor.b);
        else if (stricmp(buffer, "SnowColor") == 0)
            fscanf(in, "%f %f %f", &tod->SnowColor.r, &tod->SnowColor.g, &tod->SnowColor.b);
        else if (stricmp(buffer, "MinVisibility") == 0)
            fscanf(in, "%f", &tod->MinVis);
        else if (stricmp(buffer, "VisColor") == 0)
            fscanf(in, "%f %f %f", &tod->VisColor.r, &tod->VisColor.g, &tod->VisColor.b);
    }

    return total;
}

// Angle must be between 0 and 90
void CTimeOfDay::SetSunGlareAngle(int angle)
{
    SunGlareCosine = (float) glGetCosine(angle);
    SunGlareFactor = 1.0f / (1.0f - SunGlareCosine);
}

float CTimeOfDay::GetSunGlare(int yaw, int pitch)
{
    // TODO: Instead of all this, just do a dot product with the light vector...
    int pitch1 = GetSunPitch();
    int yaw1 = GetSunYaw();
    float sin1, sin2, cos1, cos2, cos3;
    glGetSinCos(&sin1, &cos1, pitch);
    glGetSinCos(&sin2, &cos2, pitch1);
    cos3 = (float) glGetCosine(yaw - yaw1);
    float alpha = sin1 * sin2 + cos1 * cos2 * cos3;

    alpha -= SunGlareCosine;
    alpha *= SunGlareFactor;

    // just to make sure, clamp value
    if (alpha > 1.0f) alpha = 1.0f;
    else if (alpha < 0.0f) alpha = 0.0f;

    return alpha;
}

// return 1 if don't need to blend moon, else return moon blend value
float CTimeOfDay::CalculateMoonBlend(float glare)
{
    if (IMoonPitch < 0) return 1.0f;

    float alpha = 0.0f;

    if (ISunPitch >= 0)
    {
        int pitch = GetMoonPitch();
        int yaw = GetMoonYaw();
        int pitch1 = GetSunPitch();
        int yaw1 = GetSunYaw();
        float sin1, sin2, cos1, cos2, cos3;
        glGetSinCos(&sin1, &cos1, pitch);
        glGetSinCos(&sin2, &cos2, pitch1);
        cos3 = (float) glGetCosine(yaw - yaw1);
        alpha = sin1 * sin2 + cos1 * cos2 * cos3;
        alpha -= SunGlareCosine;
        alpha *= SunGlareFactor;

        if (alpha > 1.0f) alpha = 1.0f;
        else if (alpha < 0.0f) alpha = 0.0f;

        if (sin2 > 0.0f)
        {
            //sin2 *= 4.0f;
            alpha += sin2;
        }
    }

    if (glare < 0.0f) glare = 0.0f;
    else if (glare > 1.0f) glare = 1.0f;

    alpha = 1.0f - (alpha + glare);

    if (alpha < 0.0f) alpha = 0.0f;
    else if (alpha > 1.0f) alpha = 1.0f;

    return alpha;
}

int CTimeOfDay::CalculateMoonPercent(void)
{
    if (MoonPhase == -1)
        MoonPhase = FloatToInt32(TheStar.GetMoonPhase() * MOON_PHASE_SIZE);

    return MoonPhase;
}

void CTimeOfDay::CreateMoonPhaseMask(unsigned char *image, int phase)
{
    if (phase == NEW_MOON_PHASE)  // new moon --> all moon dark
        memset((void *) image, 0, 8 * 64);
    else   // part of moon dark
    {
        int array[64];

        int reverse = 0;
        int sizex = NEW_MOON_PHASE / 2 - phase;

        if (phase > NEW_MOON_PHASE)
        {
            sizex += NEW_MOON_PHASE;
            reverse = 1;
        }

        int counter = 0;
        int flag = 1;
        int x = 0;
        int y = 32;
        int xpos = 32;
        float aa = (float) sizex * sizex;
        float bb = (float) 32 * 32;
        float d1 = bb - aa * 32 + aa / 4.0f;

        while (aa * ((float) y - 0.5f) > bb * ((float) x + 1.0f))
        {
            if (d1 < 0.0f)
            {
                d1 += bb * ((float)(x << 1) + 3.0f);
                x++;
                xpos++;
            }
            else
            {
                if (flag)
                {
                    array[counter++] = xpos;
                    flag = 0;
                }

                d1 += bb * ((float)(x << 1) + 3.0f) + aa * ((float)(-y << 1) + 2.0f);
                x++;
                xpos++;
                y--;
                array[counter++] = xpos;
            }
        }

        float x1 = (float) x + 0.5f;
        float y1 = (float) y - 1.0f;
        float d2 = bb * x1 * x1 + aa * y1 * y1 - aa * bb;

        while (y > 0)
        {
            if (d2 < 0.0f)
            {
                if (flag)
                {
                    array[counter++] = xpos;
                    flag = 0;
                }

                d2 += bb * ((float)(x << 1) + 2.0f) + aa * ((float)(-y << 1) + 3.0f);
                x++;
                xpos++;
                y--;
            }
            else
            {
                d2 += aa * ((float)(-y << 1) + 3.0f);
                y--;
            }

            array[counter++] = xpos;
        }

        int j;

        for (j = 0; j < 32; j++) array[63 - j] = array[j];

        if (sizex < 0)
        {
            for (j = 0; j < 64; j++) array[j] = 64 - array[j];
        }

        int row, col, col1;
        int start, stop;
        unsigned char *dest = image;

        for (row = 0; row < 64; row++)
        {
            if (reverse)
            {
                start = 0;
                stop = array[row];
            }
            else
            {
                start = array[row];
                stop = 64;
            }

            int col2 = 0;

            for (col = 0; col < 8; col++)
            {
                unsigned char c = 0;

                for (col1 = 0; col1 < 8; col1++)
                {
                    c <<= 1;

                    if ((col2 < start) or (col2 >= stop)) c or_eq 1;

                    col2++;
                }

                *dest++ = c;
            }
        }
    }
}

void CTimeOfDay::RotateMoonMask(int angle)
{
    float sine, cosine;
    glGetSinCos(&sine, &cosine, -angle);

    int u1[3], v1[3];

    float c32, s32;
    c32 = 65536.0f * 32.0f * cosine;
    s32 = 65536.0f * 32.0f * sine;
    u1[0] = (int)(-c32 + s32 + 65536.0f * 32.0f);
    v1[0] = (int)(-s32 - c32 + 65536.0f * 32.0f);
    u1[1] = (int)(c32 + s32 + 65536.0f * 32.0f);
    v1[1] = (int)(s32 - c32 + 65536.0f * 32.0f);
    u1[2] = (int)(-c32 - s32 + 65536.0f * 32.0f);
    v1[2] = (int)(-s32 + c32 + 65536.0f * 32.0f);

    int i, j, k;
    unsigned char *dest = CurrentMoonPhaseMask;
    int uu = u1[0];
    int vv = v1[0];
    int duu = (u1[1] - u1[0]) >> 6;
    int duv = (v1[1] - v1[0]) >> 6;
    int dvu = (u1[2] - u1[0]) >> 6;
    int dvv = (v1[2] - v1[0]) >> 6;

    for (i = 0; i < 64; i++)
    {
        int uuu = uu;
        int vvv = vv;

        for (j = 0; j < 8; j++)
        {
            unsigned char c1 = 0;

            for (k = 0; k < 8; k++)
            {
                int tu = uuu >> 16;
                int tv = vvv >> 16;
                c1 <<= 1;
                uuu += duu;
                vvv += duv;

                if (tu >= 0 and tu < 64 and tv >= 0 and tv < 64)
                {
                    int l = (tv << 3) + (tu >> 3);
                    unsigned char c = (unsigned char)(1 << (7 - (tu bitand 7)));

                    if (MoonPhaseMask[l] bitand c) c1 or_eq 1;
                }
            }

            *dest++ = c1;
        }

        uu += dvu;
        vv += dvv;
    }
}

void CTimeOfDay::CalculateMoonPhase()
{
    int angle = 0;
    float dy = SunCoord.y - MoonCoord.y;

    if (dy > 1.0f)
    {
        dy -= 2.0f;
        angle = -4096;
    }
    else if (dy < -1.0f)
    {
        dy += 2.0f;
        angle = 4096;
    }

    if (dy < 0) angle -= 4096;
    else angle += 4096;

    float dz = SunCoord.z - MoonCoord.z;
    angle += FloatToInt32(radtoangle((float)atan2(dz, dy)));
    //MI moon phase fix
#if 0
    dy = MoonCoord.z;
    dz = MoonCoord.y;
#else
    dy = -MoonCoord.y;
    dz = -MoonCoord.z;
#endif
    int angle1 = FloatToInt32(radtoangle((float)atan2(dz, dy)));
    angle -= angle1;

    RotateMoonMask(angle);
}

void CTimeOfDay::CreateMoonPhase(unsigned char *src, unsigned char *dest)
{
    int i, j, k;
    unsigned char *mask = CurrentMoonPhaseMask;

    for (i = 0; i < 64; i++)
    {
        for (j = 0; j < 8; j++)
        {
            unsigned char c = *mask++;

            for (k = 0; k < 8; k++)
            {
                unsigned char c1 = *src++;
#ifdef USE_TRANSPARENT_MOON

                if (c1 and not (c bitand 0x80)) c1 = 0;

#else

                if (c1 and not (c bitand 0x80)) c1 += 48;

#endif
                c <<= 1;
                *dest++ = c1;
            }
        }
    }
}

DWORD CTimeOfDay::MakeColor(Tcolor *col)
{
    return
        (FloatToInt32(col->r * 255.9f) bitand 0xFF) |
        ((FloatToInt32(col->g * 255.9f) bitand 0xFF) <<  8) |
        ((FloatToInt32(col->b * 255.9f) bitand 0xFF) << 16) |
        0xff000000;
}
