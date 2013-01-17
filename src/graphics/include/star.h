#ifndef _EJ_STAR_H_
#define _EJ_STAR_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifndef STAND_ALONE
#include "grTypes.h"
#endif

#define	NEW(type)				((type *) malloc(sizeof (type)))
#define	NEWARRAY(type,num)		((type *) malloc(sizeof (type) * num))
#define FREE(mem)				(free ((void *) mem))

#ifndef MAXSTRING
#define	MAXSTRING	300
#endif

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#define degtorad(d)		((d)*PI/180.0f)
#define degtohour(d)	((d)*24.0f/360.0f)

#define hourtorad(h)	((h)*PI/12.0f)
#define hourtodeg(h)	((h)*360.0f/24.0f)

#define radtodeg(r)		((r)*180.0f/PI)
#define radtohour(r)	((r)*12.0f/PI)

#define radtoangle(r)	((r)*8192.0f/PI)
#define angletorad(r)	((r)*PI/8192.0f)

#define	STAR_BEHIND_SUN		1
#define	STAR_BEHIND_MOON	2

struct StarRecord {
	float	ra, dec;
	int		color;
};

struct StarCoord {
	float	x, y, z;
	int		color;
	float	az, alt;
	int		flag;
};

struct StarData {
	int			totalstar, totalcoord;
	StarRecord	*star;
	StarCoord	*coord;
};

class CStar {
protected:

static float Latitude, Longitude, sinLatitude, cosLatitude;
static float UniversalTime, UniversalTimeDegree;
static float deltaJulian, CurrentJulian, Julian1980, Julian2000;
static float LocalSiderialTime;
static int Year, Month, Day, ExtraDay;
static int mustSetLocalSiderialTime, mustSetdeltaJulian;
static int DaysInMonth[12];
static StarData	*CurrentStarData;
static float SunAz, SunAlt, MoonAz, MoonAlt;
static float Horizon, HorizonRange, IntensityRange;
static int	minStarIntensity;

static void CalculateDeltaJulian ();
static void CalculateLocalSiderialTime ();
static int	CalculateStarCoord (float ra, float dec, StarCoord *star);
static float Julian (int year, int month, float day);
static float FixAngle (float N);
static float Kepler (float m, float ecc);
static float GetRange (float angle);
static float GetRangeRad (float angle);
static int InsideRange (float starpos, float pos);

#ifdef STAND_ALONE
static int GetMaxDay (int month, int year);
static void ConvertLocation (char *string, float loc, char c);

public:
static int GetTime (int *hour, int *minute, float *second, float timezone=0.0f);
static void GetDateTime (char *string, float timezone=0.0f);
static void GetLatitude (char *string);
static void GetLongitude (char *string);
static void GetLocation (char *string);
static float ConvertUnit (float deg, float min = 0.0f, float sec = 0.0f);
static void UpdateTime (int hour, int minute, float second = 0.0f);
static void UpdateTime (float hour);
static void UpdateTime (unsigned int mseconds);
static void CalculateSunCoord (float *x, float *y, float *z);
static void CalculateMoonCoord (float *x, float *y, float *z);
#endif

public:
CStar () {};
virtual ~CStar() { Cleanup (); };

static int Setup (char *starfile, float maxmagnitude = 12.0f);
static void Cleanup ();
static StarData *GetStarData () { return CurrentStarData; };

static int LeapYear (int year);
static int GetTotalDay (int month, int year);
static float ConvertHour (int hour, int min = 0, float sec = 0.0f);
static void CalculateDate (int *day, int *month, int *year, int extraday = 0);

static void SetDate (int day, int month=1, int year=2000);
static void SetUniversalTime (unsigned int mseconds);
static void SetUniversalTime (float hour);
static void SetUniversalTime (int hour, int minute, float second);
static void SetLocation (float latitude=38.0f, float longitude=0.0f);
static void SetHorizon (float horizon, float range = 0.0f);

static void UpdateStar ();

static void GetSunRaDec (float *ra, float *dec);
static void GetMoonRaDec (float *ra, float *dec);
static void	ConvertCoord (float ra, float dec, float *x, float *y, float *z);
static void ConvertPosition (float ra, float dec, float *az, float *alt);

static void CalculateSunPosition (float *az, float *alt);
static void CalculateMoonPosition (float *az, float *alt);
static float GetMoonPhase ();

static void RemoveDimStar (float minintensity);
static void SetSunPosition (float az, float alt) { SunAz = az; SunAlt = alt; };
static void SetMoonPosition (float az, float alt) { MoonAz = az; MoonAlt = alt; };
static void GetSunPosition (float *az, float *alt) { *az = SunAz; *alt = SunAlt; };
static void GetMoonPosition (float *az, float *alt) { *az = MoonAz; *alt = MoonAlt; };

};

#endif