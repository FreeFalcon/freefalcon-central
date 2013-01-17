#include "Star.h"
#include "Falclib\Include\openfile.h"
#define EPSILON		1e-6f
#define elonge      278.833540f     /* Ecliptic longitude of the Sun at epoch 1980.0 */
#define elongp      282.596403f     /* Ecliptic longitude of the Sun at perigee */
#define eccent      0.016718f       /* Eccentricity of Earth's orbit */
#define mmlong      64.975464f      /* Moon's mean longitude at the epoch */
#define mmlongp     349.383063f     /* Mean longitude of the perigee at the epoch */
#define mlnode      151.950429f     /* Mean longitude of the node at the epoch */
#define minc        5.145396f       /* Inclination of the Moon's orbit */

#pragma warning(disable : 4127)

float CStar::Latitude;
float CStar::Longitude;
float CStar::sinLatitude;
float CStar::cosLatitude;
float CStar::UniversalTime = 0.0f;
float CStar::UniversalTimeDegree = 0.0f;
float CStar::LocalSiderialTime = 0.0f;
float CStar::deltaJulian = 0.0f;
float CStar::CurrentJulian = 0.0f;
float CStar::Julian1980 = Julian (1980,1,0.0f);
float CStar::Julian2000 = Julian (2000,1,1.5f);
int CStar::Year;
int CStar::Month;
int CStar::Day;
int CStar::ExtraDay = 0;
int CStar::mustSetLocalSiderialTime = 0;
int CStar::mustSetdeltaJulian = 0;
int CStar::DaysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
StarData *CStar::CurrentStarData = 0;
float CStar::SunAz;
float CStar::SunAlt;
float CStar::MoonAz;
float CStar::MoonAlt;
float CStar::Horizon = (float) (-PI/2);			// default ==> show all stars
float CStar::HorizonRange = (float) (-PI/2);	// default ==> don't dim stars
float CStar::IntensityRange = 1.0f;
int CStar::minStarIntensity = 0;


#ifdef STAND_ALONE
//#pragma warning(disable : 4035)
//#pragma warning(disable : 4244)

/*
inline int FloatToInt32(float x) 
{
	__asm { 
		fld   dword ptr [x]; 
		fistp dword ptr [x]; 
		mov eax, dword ptr [x]; 
	}
	return x;
}
*/
//#pragma warning(default : 4244)
//#pragma warning(default : 4035)

int CStar::GetTime (int *hour, int *minute, float *second, float timezone)
{
	float curtime = UniversalTime * 24.0f + timezone;
	int inc = 0;
	if (curtime < 0) {
		curtime += 24.0f;
		inc = -1;
	}
	else if (curtime >= 24.0f) {
		curtime -= 24.0f;
		inc = 1;
	}
	*hour = FloatToInt32(curtime);
	curtime = (curtime - *hour) * 60.0f;
	*minute = FloatToInt32(curtime);
	*second = (curtime - *minute) * 60.0f;
	return inc;
}

void CStar::GetDateTime (char *string, float timezone)
{
	static char *monthName[12] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	int hour, minute;
	float	second;
	int day = Day;
	int month = Month;
	int year = Year;
	int inc = GetTime (&hour, &minute, &second, timezone);
	CalculateDate (&day, &month, &year, ExtraDay+inc);
	sprintf(string, "%s %d, %d %2d:%02d:%02.1f", monthName[month-1], day, year, hour, minute, second);
}

void CStar::ConvertLocation (char *string, float loc, char c)
{
	if (loc < 0.0f) loc = -loc;
	loc = radtodeg(loc);
	int lat = FloatToInt32 (loc);
	loc = (loc - lat) * 60.0f;
	int min = FloatToInt32(loc);
	float sec = (loc - min) * 60.0f;
	if (sec >= 0.1f) {
		sprintf (string, "%2d deg %2d\" %02.1f\' %c", lat, min, sec, c);
	}
	else if (min > 0) {
		sprintf (string, "%2d deg %2d\" %c", lat, min, c);
	}
	else {
		sprintf (string, "%2d deg %c", lat, c);
	}
}

void CStar::GetLatitude (char *string)
{
	char c = 'N';
	if (Latitude < 0.0f) c = 'S';
	ConvertLocation (string, Latitude, c);
}

void CStar::GetLongitude (char *string)
{
	char c = 'E';
	if (Longitude < 0.0f) c = 'W';
	ConvertLocation (string, Longitude, c);
}

void CStar::GetLocation (char *string)
{
	GetLatitude (string);
	string += strlen(string);
	*string++ = ' ';
	GetLongitude (string);
}

int CStar::GetMaxDay (int month, int year)
{
	if (month == 2) {
		if (LeapYear(year)) return 29;
		else return 28;	
	}
	return DaysInMonth[month-1];
}

float CStar::ConvertUnit (float deg, float min, float sec) 
{
	float ms = min / 60.0f + sec / 3600.0f;
	if (deg < 0) return deg - ms;
	return deg + ms;
}

void CStar::UpdateTime (unsigned int mseconds)
{
	float hour = mseconds * (1.0f / (24.0f*3600000.0f));
	UpdateTime (hour);
}

void CStar::UpdateTime (int hour, int minute, float second)
{
	float curtime = ConvertHour (hour, minute, second) / 24.0f;
	UpdateTime (curtime);
}

void CStar::UpdateTime (float curtime)
{
	UniversalTime += curtime;
	if (UniversalTime >= 1.0f) {
		int	ut = FloatToInt32(UniversalTime);
		ExtraDay += ut;
		UniversalTime -= ut;
	}
	UniversalTimeDegree =  degtorad(360.0f) * UniversalTime;

	CalculateDeltaJulian ();
	CalculateLocalSiderialTime ();
}

void CStar::CalculateSunCoord (float *x, float *y, float *z)
{
	float ra, dec;
	GetSunRaDec (&ra, &dec);
	ConvertCoord (ra, dec, x, y, z);
}

void CStar::CalculateMoonCoord (float *x, float *y, float *z)
{
	float ra, dec;
	GetMoonRaDec (&ra, &dec);
	ConvertCoord (ra, dec, x, y, z);
}
#endif // STAND_ALONE


int CStar::LeapYear (int year)
{
	int leap = 0;
	if (!(year & 3)) {
		if (!(year % 400)) leap = 1;
		else if (year % 100) leap = 1;
	}
	return leap;
}

int CStar::GetTotalDay (int month, int year)
{
	if (month > 2) {
		if (LeapYear(year)) DaysInMonth[1] = 29;
		else DaysInMonth[1] = 28;
	}
	int i;
	int days = 0;
	for (i=0; i < month; i++) days += DaysInMonth[i];
	return days;
}

float CStar::ConvertHour (int hour, int min, float sec) 
{
	return hour + min / 60.0f + sec / 3600.0f;
}

void CStar::CalculateDate (int *day, int *month, int *year, int extraday)
{
	if (!extraday) return;
	int d = *day + extraday;
	int m = *month;
	int y = *year;
	if (m > 1) {
		d += GetTotalDay (m-1, y);
		m = 1;
	}
	while (1) {
		int maxday = 365 + LeapYear (y);
		if (d <= maxday) break;
		else {
			y++;
			d -= maxday;
		}
	}
	while (1) {
		if (d <= DaysInMonth[m-1]) break;
		d -= DaysInMonth[m-1];
		m++;
	}
	*day = d;
	*month = m;
	*year = y;
}


float CStar::Julian (int year, int month, float day)
{
    if (month > 2) {
        month = month - 3;
    } 
	else {
        month = month + 9;
        year--;
    }
    int c = year / 100;
    year -= (100 * c);
	c = (c * 146097) >> 2;
	year = (year * 1461) >> 2;
	month = (month * 153 + 2) / 5;
	day += (c + year + month + 1721119);
    return (day);
}

int CStar::Setup (char *starfile, float maxmagnitude)
{
	Cleanup ();
	FILE *in = FILE_Open (starfile, "r");
	if (in == NULL) return 1;

	char buffer[MAXSTRING];

	int		totalcons=0, totalstar=0;
	float	minmag=0.0F, maxmag=0.0F;
	float	minint, maxint;

	minint = 0.5f;
	maxint = 1.0f;
	fscanf (in, "%s", buffer);		// StarInfo
	while (1) {
		fscanf (in, "%s", buffer);
		strupr (buffer);
		if (!strcmp (buffer, "ZZZZ")) break;
		else if (!strcmp (buffer, "TOTALCONSTELLATION")) {
			fscanf (in, "%d", &totalcons);
		}
		else if (!strcmp (buffer, "TOTALSTAR")) {
			fscanf (in, "%d", &totalstar);
		}
		else if (!strcmp (buffer, "MINMAG")) {
			fscanf (in, "%f", &minmag);
		}
		else if (!strcmp (buffer, "MAXMAG")) {
			fscanf (in, "%f", &maxmag);
			if (maxmag > maxmagnitude) maxmag = maxmagnitude;
		}
		else if (!strcmp (buffer, "MININTENSITY")) {
			fscanf (in, "%f", &minint);
		}
		else if (!strcmp (buffer, "MAXINTENSITY")) {
			fscanf (in, "%f", &maxint);
		}
	}

	StarData *data = NEW (StarData);
	if (!data) {
		fclose (in);
		return 2;
	}

	if(!totalstar){
		fclose (in);
		return 2;
	}

	StarRecord *star = NEWARRAY (StarRecord, totalstar);
	if (!star) {
		FREE(data);
		fclose (in);
		return 2;
	}
	data -> star = star;
	data -> totalstar = totalstar;

	float	deltamag = max(0.01F, maxmag - minmag);
	float	deltaint = (maxint - minint) / deltamag;

	StarRecord	*curstar = star;
	int i, j;
	float	mag;
	for (i=0; i < totalcons;i++) {
		fscanf (in, "%s", buffer);		// Constellation
		fscanf (in, "%[^\n]", buffer);
		fscanf (in, "%s", buffer);		// TotalStar
		fscanf (in, "%d", &totalstar);
		for (j=0; j < totalstar;j++) {
			fscanf (in, "%s", buffer);	// Mag
			fscanf (in, "%f", &mag);
			fscanf (in, "%s", buffer);	// RaDec
			fscanf (in, "%f %f", &curstar -> ra, &curstar -> dec);
			curstar -> ra = hourtorad(curstar -> ra);
			curstar -> dec = degtorad(curstar -> dec);
			fscanf (in, "%s", buffer);	// ID
			fscanf (in, "%[^\n]", buffer);
			if (mag < maxmagnitude) {
				mag = (mag - minmag) * deltaint;
				if (mag > 1.0f) mag = 1.0f;	// just in case
				curstar -> color = FloatToInt32((1.0f - mag) * 255.0f);
				curstar++;
			}
			else data -> totalstar--;
		}
	}
	fclose (in);

	star = NEWARRAY (StarRecord, data -> totalstar);
	if (!star) {
		FREE(data -> star);
		FREE(data);
		return 2;
	}
	memcpy (star, data -> star, sizeof(StarRecord) * data -> totalstar);
	FREE(data -> star);
	data -> star = star;

	StarCoord *coord = NEWARRAY (StarCoord, data -> totalstar);
	if (!coord) {
		FREE(data -> star);
		FREE(data);
		return 2;
	}
	data -> coord = coord;
	data -> totalcoord = 0;

	int max;
	for (i=0; i < data -> totalstar; i++) {
		max = i;
		for (j=i + 1; j < data -> totalstar; j++) {
			if (data -> star[j].color >  data -> star[max].color) max = j;
		}
		if (i != max) {
			StarRecord tempstar = data -> star[max];
			data -> star[max] = data -> star[i];
			data -> star[i] = tempstar;
		}
	}

	minStarIntensity = 0;
	CurrentStarData = data;
	return 0;
}

void CStar::Cleanup ()
{
	if (CurrentStarData) {
		FREE(CurrentStarData -> coord);
		FREE(CurrentStarData -> star);
		FREE(CurrentStarData);
		CurrentStarData = 0;
	}
}

#define	MAXRANGE	2.5
int CStar::InsideRange (float starpos, float pos)
{
	if (starpos < pos-degtorad(MAXRANGE)) return 0;
	if (starpos > pos+degtorad(MAXRANGE)) return 0;
	return 1;
}

void CStar::UpdateStar ()
{
	StarRecord *star = CurrentStarData -> star;
	StarCoord *coord = CurrentStarData -> coord;
	CurrentStarData -> totalcoord = 0;
	int i;
	for (i=0; i < CurrentStarData -> totalstar; i++, star++) {
		if (star -> color < minStarIntensity) continue;	// skip dim star
		if (CalculateStarCoord (star -> ra, star -> dec, coord)) {
			if (InsideRange(coord -> az, SunAz) && InsideRange(coord -> alt, SunAlt))
				coord -> flag |= STAR_BEHIND_SUN;
			if (InsideRange(coord -> az, MoonAz) && InsideRange(coord -> alt, MoonAlt))
				coord -> flag |= STAR_BEHIND_SUN;
			if (coord -> alt < HorizonRange) {
				float intensity = (coord -> alt - Horizon) * IntensityRange;
				coord -> color = FloatToInt32(intensity * star -> color);
			}
			else coord -> color = star -> color;
			coord++;
			CurrentStarData -> totalcoord++;
		}
	}
}

float CStar::GetRange (float angle)
{
	while (angle >= 360.0f) angle -= 360.0f;
	while (angle < 0) angle += 360.0f;
	return angle;
}

float CStar::GetRangeRad (float angle)
{
	while (angle >= PI*2) angle -= PI*2;
	while (angle < 0) angle += PI*2;
	return angle;
}

int CStar::CalculateStarCoord (float ra, float dec, StarCoord *star)
{
	if (mustSetdeltaJulian) CalculateDeltaJulian ();
	if (mustSetLocalSiderialTime) CalculateLocalSiderialTime ();
	float HourAngle = GetRangeRad(LocalSiderialTime - ra);
	float sinDEC = (float) sin(dec);
	float cosDEC = (float) cos(dec);
	float cosHA = (float) cos(HourAngle);
	float sinHA = (float) sin(HourAngle);
	float sinALT = sinDEC * sinLatitude + cosDEC * cosLatitude * cosHA;
	float altitude = (float) asin(sinALT);
	if (altitude < Horizon) return 0;
	float cosALT = (float) cos(altitude);
	float azimuth = (float) acos((sinDEC - sinALT * sinLatitude) / (cosALT * cosLatitude));
	if (sinHA >= 0) azimuth = 2*PI - azimuth;
	float sinAZ = (float) sin(azimuth);
	float cosAZ = (float) cos(azimuth);

	star -> az = (float) azimuth;
	star -> alt = (float) altitude;
	star -> flag = 0;

// X = North Y = East Z = Down
	star -> x = (float) (cosAZ * cosALT);
	star -> y = (float) (sinAZ * cosALT);
	star -> z = (float) (-sinALT);
	return 1;
}

void CStar::ConvertPosition (float ra, float dec, float *az, float *alt)
{
	float HourAngle = GetRangeRad(LocalSiderialTime - ra);
	float sinDEC = (float) sin(dec);
	float cosDEC = (float) cos(dec);
	float cosHA = (float) cos(HourAngle);
	float sinHA = (float) sin(HourAngle);
	float sinALT = sinDEC * sinLatitude + cosDEC * cosLatitude * cosHA;
	float altitude = (float) asin(sinALT);
	float cosALT = (float) cos(altitude);
	float azimuth = (float) acos((sinDEC - sinALT * sinLatitude) / (cosALT * cosLatitude));
	if (sinHA >= 0) azimuth = 2*PI - azimuth;

	*az = (float) azimuth;
	*alt = (float) altitude;
}

void CStar::ConvertCoord (float ra, float dec, float *x, float *y, float *z)
{
	float HourAngle = GetRangeRad(LocalSiderialTime - ra);
	float sinDEC = (float) sin(dec);
	float cosDEC = (float) cos(dec);
	float cosHA = (float) cos(HourAngle);
	float sinHA = (float) sin(HourAngle);
	float sinALT = sinDEC * sinLatitude + cosDEC * cosLatitude * cosHA;
	float altitude = (float) asin(sinALT);
	float cosALT = (float) cos(altitude);
	float azimuth = (float) acos((sinDEC - sinALT * sinLatitude) / (cosALT * cosLatitude));
	if (sinHA >= 0) azimuth = 2*PI - azimuth;
	float sinAZ = (float) sin(azimuth);
	float cosAZ = (float) cos(azimuth);

// X = North Y = East Z = Down
	*x = (float) (cosAZ * cosALT);
	*y = (float) (sinAZ * cosALT);
	*z = (float) (-sinALT);
}

void CStar::SetLocation (float latitude, float longitude)
{
	Latitude = degtorad(latitude);
	Longitude = degtorad(longitude);
	sinLatitude = (float) sin(Latitude);
	cosLatitude = (float) cos(Latitude);
	mustSetLocalSiderialTime = 1;
}

void CStar::SetDate (int day, int month, int year)
{
	Year = year;
	Month = month;
	Day = day;
	ExtraDay = 0;
	mustSetdeltaJulian = 1;
}

void CStar::SetUniversalTime (float curtime)
{
	UniversalTime = curtime;
	if (UniversalTime >= 1.0f) {
		ExtraDay = FloatToInt32(UniversalTime);
		UniversalTime -= ExtraDay;
	}
	UniversalTimeDegree =  degtorad(360.0f) * UniversalTime;
	CalculateDeltaJulian ();
	CalculateLocalSiderialTime ();
}

void CStar::SetUniversalTime (int hour, int minute, float second)
{
	float curtime = ConvertHour (hour, minute, second) / 24.0f;
	SetUniversalTime (curtime);
}

void CStar::SetUniversalTime (unsigned int mseconds)
{
	float hour = mseconds * (1.0f / (24.0f*3600000.0f));
	SetUniversalTime (hour);
}

void CStar::CalculateLocalSiderialTime ()
{
	LocalSiderialTime = GetRangeRad(degtorad(100.46f) + degtorad(0.985647f) * deltaJulian + Longitude + UniversalTimeDegree);
	mustSetLocalSiderialTime = 0;
}

void CStar::CalculateDeltaJulian ()
{
	int year = Year;
	int month = Month;
	int day = Day;
	CalculateDate (&day, &month, &year, ExtraDay);
	CurrentJulian = Julian (year, month, day + UniversalTime);
	deltaJulian = CurrentJulian - Julian2000;
	mustSetdeltaJulian = 0;
}

float CStar::FixAngle (float N)
{
	return N - 360.0f * (float) floor(N / 360.0f);
}

float CStar::Kepler (float m, float ecc)
{
	float e, delta;

	e = m = degtorad(m);
	do {
		delta = e - ecc * (float) sin(e) - m;
		e -= (delta / (1 - ecc * (float) cos(e)));
	} while (fabs(delta) > EPSILON);
	return e;
};

// 0 = full moon, 0.5 = new moon
float CStar::GetMoonPhase ()
{
	float Day = CurrentJulian - Julian1980;
	float N = FixAngle ((360.0f/365.2422f) * Day);
	float M = FixAngle(N + elonge - elongp);
	float Ec = Kepler (M, eccent);
	Ec = (float) sqrt((1 + eccent) / (1 - eccent)) * (float) tan(Ec/2);
	Ec = radtodeg(2) * (float) atan (Ec);
	float Lambdasun = FixAngle (Ec + elongp);
	float ml = FixAngle (13.1763966f * Day + mmlong);
	float MM = FixAngle (ml - 0.1114041f * Day - mmlongp);
//	float MN = FixAngle (mlnode - 0.0529539f * Day);
	float Ev = 1.2739f * (float) sin (degtorad(2 * (ml - Lambdasun) - MM));
	float sinM = (float) sin(degtorad(M));
	float Ae = 0.1858f * sinM;
	float A3 = 0.37f * sinM;
	float MmP = degtorad((MM + Ev - Ae - A3));
	float mEc = 6.2886f * (float) sin(MmP);
	float A4 = 0.214f * (float) sin (2 * MmP);
	float lP = ml + Ev + mEc - Ae + A4;
	float V = 0.6583f * (float) sin(degtorad(2) * (lP-Lambdasun));
	float MoonAge = lP + V - Lambdasun;
	MoonAge = GetRange(MoonAge+180.0f);
	return MoonAge / 360.0f;
}

void CStar::GetSunRaDec (float *ra, float *dec)
{
	float g = GetRangeRad(degtorad(357.528f) + degtorad(0.9856003f)*deltaJulian);
	float g2 = g * 2;
	float L = GetRangeRad(degtorad(280.461f) + degtorad(0.9856474f)*deltaJulian);
	float lambda = GetRangeRad(L + degtorad(1.915f) * (float) sin(g) + degtorad(0.02f) * (float) sin(g2));
	float epsilon = GetRangeRad (degtorad(23.439f) - degtorad(0.0000004f)*deltaJulian);
	float sinlambda = (float) sin(lambda);
	*ra = GetRangeRad((float) atan2 (cos(epsilon) * sinlambda, cos(lambda)));
	*dec = (float) asin (sin(epsilon) * sinlambda);
}

void CStar::GetMoonRaDec (float *ra, float *dec)
{
	float t = degtorad(deltaJulian) / 36525.0f;
	float l =								 GetRangeRad(degtorad(218.32f) + 481267.883f * t)
				+ degtorad(6.29f)*(float)sin(GetRangeRad(degtorad(134.9f)  + 477198.85f  * t))
				- degtorad(1.27f)*(float)sin(GetRangeRad(degtorad(259.2f)  - 413335.38f  * t))
				+ degtorad(0.66f)*(float)sin(GetRangeRad(degtorad(235.7f)  + 890534.23f  * t))
				+ degtorad(0.21f)*(float)sin(GetRangeRad(degtorad(269.9f)  + 954397.7f   * t))
				- degtorad(0.19f)*(float)sin(GetRangeRad(degtorad(357.5f)  +  35999.05f  * t))
				- degtorad(0.11f)*(float)sin(GetRangeRad(degtorad(186.6f)  + 966404.05f  * t));
	l = GetRangeRad(l);
	float bm =    degtorad(5.13f)*(float)sin(GetRangeRad(degtorad( 93.3f)  + 483202.03f  * t))
				+ degtorad(0.28f)*(float)sin(GetRangeRad(degtorad(228.2f)  + 960400.87f  * t))
				- degtorad(0.28f)*(float)sin(GetRangeRad(degtorad(318.3f)  +   6003.18f  * t))
				- degtorad(0.17f)*(float)sin(GetRangeRad(degtorad(217.6f)  - 407332.2f   * t));
	float gp =    degtorad(0.9508f)
				+ degtorad(0.0518f)*(float)cos(GetRangeRad(degtorad(134.9f) + 477198.85f * t))
				+ degtorad(0.0095f)*(float)cos(GetRangeRad(degtorad(259.2f) - 413335.38f * t))
				+ degtorad(0.0078f)*(float)cos(GetRangeRad(degtorad(235.7f) + 890534.23f * t))
				+ degtorad(0.0028f)*(float)cos(GetRangeRad(degtorad(269.9f) + 954397.7f  * t));
//	float sdia = 0.2725f * gp;
	float rm = 1.0f / (float) sin(gp);
	float cosbm = (float) cos(bm);
	float xg = rm * (float) cos(l) * cosbm;
	float yg = rm * (float) sin(l) * cosbm;
	float zg = rm * (float) sin(bm);
	float ecl = degtorad(23.4393f) - degtorad(3.563e-7f) * deltaJulian;
	float cosecl = (float) cos(ecl);
	float sinecl = (float) sin(ecl);
	float xe = xg;
	float ye = yg * cosecl - zg * sinecl;
	float ze = yg * sinecl + zg * cosecl;
	*ra = GetRangeRad((float) atan2(ye, xe));
	*dec = (float) atan (ze/sqrt(xe*xe + ye*ye));
}

void CStar::CalculateSunPosition (float *az, float *alt)
{
	float ra, dec;
	GetSunRaDec (&ra, &dec);
	ConvertPosition (ra, dec, &SunAz, &SunAlt);
	*az = SunAz;
	*alt = SunAlt;
}

void CStar::CalculateMoonPosition (float *az, float *alt)
{
	float ra, dec;
	GetMoonRaDec (&ra, &dec);
	ConvertPosition (ra, dec, &MoonAz, &MoonAlt);
	*az = MoonAz;
	*alt = MoonAlt;
}

void CStar::RemoveDimStar (float minintensity)
{
	minStarIntensity = (unsigned int) (minintensity * 255.0f);
}

void CStar::SetHorizon (float horizon, float range) 
{ 
	Horizon = horizon; 
	HorizonRange = horizon + range; 
	IntensityRange = 1.0f;
	if (range) IntensityRange /= range;
}
