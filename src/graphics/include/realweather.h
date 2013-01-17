/***************************************************************************\
    RealWeather.cpp
    Miro "Jammer" Torrielli
    09Nov03

	And then there was light.
\***************************************************************************/
#ifndef _REALWEATHER_H_
#define _REALWEATHER_H_

#define MAX_NUM_CLOUDS 81
#define MAX_NUM_DRAWABLES 49
#define MAX_NUM_CELLS 9
#define NUM_3DCLOUD_POLYS 20
#define NUM_3DCLOUD_POINTS 40
#define NUM_LIGHTNING_POINTS 33
#define LIGHTNING_RADIUS 15000.f
#define NUM_STRATUS_TEXTURES 4
#define NUM_CUMULUS_TEXTURES 10
#define	SKY_ROOF_HEIGHT	60000.f
#define	SKY_ROOF_RANGE 200000.f
#define	SKY_MAX_HEIGHT 70000.f

#define	NOP							_asm {nop}
#define	INT3						_asm {int 3}

#define SAFE_DELETE(p)				{ if(p) { delete (p); (p) = NULL; } }
#define SAFE_DELETE_ARRAY(p)		{ if(p) { delete[] (p); (p) = NULL; } }
#define SAFE_RELEASE(p)				{ if(p) { (p)->Release(); (p) = NULL; } }

#define	ARGB_TEXEL_SIZE				4
#define	ARGB_TEXEL_BITS				32

#define TEX_UV_LSB					1.f/1024.f
#define TEX_UV_MIN					TEX_UV_LSB
#define TEX_UV_MAX					1.f-TEX_UV_LSB

#define ONEOVER4PI					1.f/(4.f*PI)
#define THREEOVER16PI				3.f/(16.f*PI)
#define R360						.0027777777777778f
#define HTMS						3600000
#define MSTH						2.7777777777777778e-7
#define	HTD							15
#define HTR							.2617993877991494f

#define PHIMAX						91
#define HORIZONPHI					86
#define THETAMAX					361
#define SKYTEXTURESIZE				512
#define MAXLEVELS					5
#define CUMULUSPOLYS				20
#define CUMULUSPOINTS				40
#define LIGHTNINGPOINTS				33
#define LIGHTNINGRADIUS				15000.f
#define STRATUSTEXTURES				4
#define CUMULUSTEXTURES				10
#define	SHADOWCELL					2
#define	DRAWABLECELL				1
#define NUMCELLS					9
#define CELLSIZE					57344
#define HALFCELLSIZE				CELLSIZE/2
#define HALFCELLS					(NUMCELLS-1)/2
#define VPSHIFT						HALFCELLS*CELLSIZE
#define PUFFRADIUS					1500.f
#define CUMULUSRADIUS				10000.f
#define STRATUSRADIUS				80000.f
//#define NVG_LIGHT_LEVEL				.703125f
#define NUMSTARS					1348
#define	MAXRANGE					2.5f
#define HALF_PI					1.570796326795F

#define	MIN_WEATHER_Q_STEP		0.0001f
#define	MAX_WEATHER_Q_STEPS		1000

#define CLOUDS_FIX 1 // RV - I-Hawk - the clouds fix


#include "fmath.h"
#include "RenderOW.h"
#include "real3DCloud.h"
#include "real2DCloud.h"

typedef signed long SLONG;

enum { SUNNY = 1, FAIR, POOR, INCLEMENT, NUMCONDITIONS };
enum { OBSERVER_LOW = 0, OBSERVER_MIDDLE, OBSERVER_HI, MAX_OBSERVER_POSITIONS };

struct WeatherCell
{
	BOOL onScreen;
	Tpoint shadowPos;
	float cloudPosX,cloudPosY;
	int sTxtIndex,cTxtIndex[NUM_3DCLOUD_POLYS],cPntIndex[NUM_3DCLOUD_POLYS];
	float Radius;
};

class RealWeather
{
public:
	RealWeather();
	virtual ~RealWeather();

public:
	void Setup(ObjectDisplayList* cumulusList=NULL,ObjectDisplayList* stratusList=NULL);
	void SetRenderer(class RenderOTW *Renderer);
	void RefreshWeather(class RenderOTW *Renderer);
	void UpdateLighting();
	void Draw();
	void DrawCumulus(Tpoint *position,int txtIndex, float Radius);
	void DrawStratus(Tpoint *position,int txtIndex=0);
	void DrawStratus2(Tpoint *position,int txtIndex=0);
	void SetGreenMode(BOOL state);
	void SetupTexturesOnDevice(DXContext *rc);
	void ReleaseTexturesOnDevice(DXContext *rc);
	void Cleanup();
	void UpdateCondition(void);

protected:
	void GenerateClouds(bool bRandom=TRUE);
	void UpdateCells();
	void UpdateDrawables();
	void DrawRain();
	void DoLightning();
	void GenerateCloud(DWORD row, DWORD col);

public:
	BOOL isLightning;
	RenderOTW *renderer;
	Tpoint lightningSkyPos;
	Tpoint lightningGroundPos;
	Texture overcastTexture;
	Texture CirrusCumTextures;
	Texture CumulusTextures;
	WeatherCell weatherCellArray[MAX_NUM_CELLS][MAX_NUM_CELLS];
	int numCells,halfCells,cellSize,shadowCell,drawableCell,halfSize,vpShift,weatherCondition,weatherShiftX,weatherShiftY;
	float viewerX,viewerY,viewerZ,cumulusZ,stratusZ,stratus2Z,windSpeed,windHeading,puffRadius,cloudRadius,stratusRadius,stratusDepth,ShadingFactor;
	float HiOvercast, LoOvercast, MidOvercast;
	int numMETARS;//Cobra
	bool drRain;
	Tpoint	WindVector;
	Tpoint	GetWindVector(void) { return WindVector; }
	bool	LinearFog(void) { return LinearFogStatus; }
	void	LinearFog(bool Status) { LinearFogStatus = Status; }
	float	LinearFogEnd(void) { return LinearFogUsed; }
	void	LinearFogEnd(float Limit) { LinearFogLimit = Limit; }
	float	VisibleLimit(void) { return VisibleHeight; }
	void	SetDrawingOrder(float ZPosition);

	bool	InsideOvercast() { return InsideOVCST; }
	bool	UnderOvercast() { return UnderOVCST; }
	bool	OverOvercast() { return OverOVCST; }

protected:
	int oldWeatherCondition;
	Real2DCloud *real2DClouds;
	Real3DCloud	*real3DClouds;
	Tpoint lightningPos,lightVector;
	DWORD oldTimeMS,startMS,intervalMS;
	Texture rainTexture,lightningTexture;
	BOOL bSetup,greenMode,belowLayer,insideLayer,updateLighting,drawLightning,didOnce;
	float lZM,lRad,lDist,rainX,rainY,rainZ,sunMag,sunAngle,sunYaw;
	DWORD	CloudLoColor, CloudHiColor;
	DWORD	Stratus1Color, Stratus2Color;
	DWORD	GetObserverOrder(float ZPosition);	

	float	LinearFogLimit, LinearFogUsed;							// The Linear Fog range
	bool	LinearFogStatus;										// The Linear Fog enabled status
	float	VisibleHeight;											// The Min Height an object must be for being visible
																	// used for Bad weather conditions to avoid drawing objects
																	// under the overcast layer
	static	float	WeatherQuality, WeatherQualityRate;				// Weather variation
	static	DWORD	WeatherQualityStep, WeatherQualityElapsed;
	
	Tpoint	LastViewPos;
	float	LinearFogDelta;
	DWORD	LastTime;

	bool	UnderOVCST, OverOVCST, InsideOVCST;

	static Tcolor litCloudColor;

protected:
	static	void TimeUpdateCallback(void *unused);
	static	void UpdateWeatherQuality(void);
	//Cobra
	bool ReadWeather(void);
	typedef struct METAR
	{
		char station [4];
		int time;
		int windDirection;
		int windSpeed;
		int visibility;
		int cldType;//CB TCU
		int skyCoverage[5];
		int skyCoverageAlt[5];
		int weatherType;
		int altimeter;
	} METAR;//End Cobra
public:
	METAR *metar;
};

extern RealWeather *realWeather;

/*
class Weather
{
public:
	Weather();
	~Weather(){};

	void Init();

	void Weather::Cleanup()
	{
		m_bRenderedFirstFrame = 0;

		m_fSkyCurY = 0;
		SkyTexture.FreeTexture();
		m_pSkyTexture = (DWORD *)SkyTexture.imageData;
	}

	struct StarRecord
	{
		float x;
		float y;
		float z;
		float ra;
		float dec;
		float az;
		float alt;
		int	flag;
		int	color;
	};

	int	ThereIsASun()
	{
		return(m_fSunPitch > 0.f);
	}
	
	int	ThereIsAMoon()
	{
		return(m_fMoonPitch > 0.f);
	}

	int GetCondition()
	{
		return m_nCondition;
	}

	BOOL ConditionIsLocked()
	{
		return m_bLockedCondition;
	}

	BOOL ConditionIsUnlockable()
	{
		return m_bUnlockableCondition;
	}

	float GetLightLevel()
	{
		return m_fAmbient+m_fDiffuse;
	}
	
	float GetLightIntensity()
	{
		return m_fLightMod*(m_fAmbient+max(-m_vLightVector.z*m_fDiffuse,0.f));
	}
	
	void GetLightDirection(Tpoint *vec)
	{
		memcpy(vec,&m_vLightVector,sizeof(Tpoint));
	}

	float GetAmbientValue()
	{
		return m_fAmbient;
	}
	
	float GetDiffuseValue()
	{
		return m_fDiffuse;
	}
	
	float GetSpecularValue()
	{
		return m_fSpecular;
	}
	
	float GetStarIntensity()
	{
		return m_fStarIntensity;
	}

	void GetSunPosition(float *az,float *alt)
	{
		*az = m_fSunAzimuth; *alt = m_fSunAltitude;
	}
	
	void GetMoonPosition(float *az,float *alt)
	{
		*az = m_fMoonAzimuth; *alt = m_fMoonAltitude;
	}

	void SetSunPosition(float az,float alt)
	{
		m_fSunAzimuth = az; m_fSunAltitude = alt;
	}
	
	void SetMoonPosition(float az,float alt)
	{
		m_fMoonAzimuth = az; m_fMoonAltitude = alt;
	}

	float GetSunYaw()
	{
		float yaw = m_fSunYaw; if(m_fSunPitch > HALF_PI) yaw += PI; return yaw;
	}
	
	float GetMoonYaw()
	{
		float yaw = m_fMoonYaw; if(m_fMoonPitch > HALF_PI) yaw += PI; return yaw;
	}
	
	float GetSunPitch()
	{
		float pitch = m_fSunPitch; if(pitch > HALF_PI) pitch = PI - pitch; return pitch;
	}
	
	float GetMoonPitch()
	{
		float pitch = m_fMoonPitch; if(pitch > HALF_PI) pitch = PI - pitch; return pitch;
	}

	float GetWindHeading(Tpoint *pos=NULL) 
	{ 
		return m_fWindHeading; 
	}

	float GetWindSpeed(Tpoint *pos=NULL) 
	{ 
		return m_fWindSpeed; 
	}

	float GetWindSpeedFPS(Tpoint *pos=NULL) 
	{ 
		return m_fWindSpeed*.9113f; 
	}

	float GetTemperature() 
	{ 
		return m_fTemperature; 
	}

	float GetTemperature(Tpoint *pos) 
	{ 
		return m_fTemperature - 3*(-pos->z/1000.f); 
	}

	float GetStratusZ() 
	{ 
		return m_fStratusZ; 
	}

	float GetCumulusZ() 
	{ 
		return m_fCumulusZ; 
	}

	int GetStratusBase() 
	{ 
		return m_nStratusBase; 
	}

	float GetContrailLow() 
	{ 
		return m_fContrailLow; 
	}

	float GetContrailHigh() 
	{ 
		return m_fContrailHigh; 
	}

	int	GetCloudCover(float x,float y) 
	{ 
		return 0; 
	};
	
	int	GetCloudLevel(float x,float y) 
	{ 
		return 0; 
	}

	float GetWeatherShiftX()
	{
		return m_fWeatherShiftX;
	}

	float GetWeatherShiftY()
	{
		return m_fWeatherShiftY;
	}

	BOOL IsThereLightning() 
	{ 
		return m_bIsLightning; 
	}

	Tpoint *GetLightningGroundPosition() 
	{ 
		return &m_vLightningGroundPos; 
	}
	
	void SetWindHeading(float hdg) 
	{ 
		m_fWindHeading = hdg; 
	}

	void SetWindSpeed(float speed) 
	{ 
		m_fWindSpeed = speed; 
	}
	
	void SetTemperature(float temp)
	{ 
		m_fTemperature = temp; 
	}

	void SetStratusBase(int base) 
	{ 
		m_nStratusBase = base; 
	}

	void SetLightMod(float light) 
	{ 
		m_fLightMod = light; 
	}

	void SetLockedCondition(BOOL lock) 
	{ 
		m_bLockedCondition = lock; 
	}

public:
	StarRecord StarTable[NUMSTARS];
	WeatherCell CellArray[NUMCELLS][NUMCELLS];

protected:
	void DrawFlare();
	void DrawRain();
	void DrawMoon();
	void DrawStars();
	void DrawClouds();
	void DrawLightning();
	void DrawCumulus(Tpoint*,float,float,float uvStep=1.f);
	void DrawStratus(Tpoint*,float,float,float uvStep=1.f);
	void UpdateMoon();
	void UpdateStars();
	void UpdateCells();
	void UpdateShadows();
	void UpdateLighting();
	void UpdateSkyLighting();
	void GenerateClouds();
	void GenerateSkySphere();
	void LoadSkyNormalMap(char*);
	void GenerateSkyNormals();
	void ComputeSunPosition(float*,float*);
	void ComputeMoonPosition(float*,float*);
	void ComputeSunGroundPos(Tpoint*);
	void ComputeSunMoonPosition(Tpoint*,BOOL isMoon=FALSE);
	void ComputeMoonPhase();
	int ComputeMoonPercent();
	void ComputeDeltaJulian();
	void ComputeDate(int*,int*,int*,int doy=0);
	void ComputeLocalSiderialTime();
	void RotateMoonMask(float angle);
	void CreateMoonPhase(BYTE*,BYTE*);
	void CreateMoonPhaseMask(BYTE*,int);
	void SetDate(int,int month=1,int year=2000);
	void SetUniversalTime(float time);
	void SetUniversalTime(int mseconds);
	void SetUniversalTime(int,int,float);
	float GetMoonPhase();
	int GetTotalDays(int,int);
	void GetSunRaDec(float*,float*);
	void GetMoonRaDec(float*,float*);
	int	IsLeapYear(int);
	float GetKepler(float,float);
	float GetJulian(int,int,float);
	void RaDecToXYZ(float,float,float*,float*,float*);
	void RaDecToAzAlt(float,float,float*,float*);
	float ComputeOpticalLength(float);
	color ComputeInScattering(float,float,float);
	color ComputeExtinction(float,float);
	void ComputeScatteringConstants();
	void ComputeSunAttenuation(float);
	color ComputeSkyColor(Tpoint*);
	bool SaveDDSA8R8G8B8(char*,BYTE*,int,int);

	static void TimeUpdateCallback(void*);

protected:
	int	m_nCondition;
	int m_nLastCheck;
	BOOL m_bLockedCondition;
	BOOL m_bNeedsWeatherRefresh;
	BOOL m_bUnlockableCondition;
	int m_bRenderedFirstFrame;	
	int	m_nWeatherDay;
	int	m_nStratusBase;
	int	m_nCumulusBase;
	float m_fTemperature;
	int m_dwContrailBase;
	float m_fContrailLow;
	float m_fContrailHigh;
	float m_fCumulusZ;
	float m_fStratusZ;
	float m_fWindSpeed;
	float m_fWindHeading;
	float m_fInverseSunYaw;
	float m_fViewerX;
	float m_fViewerY;
	float m_fViewerZ;
	float m_fFogStart;
	float m_fFogEnd;
	float m_fFogDepth;
	float m_fHazeMax;
	float m_fHazeTop;
	float m_fRHazeTop;
	float m_fHorizonBlend;
	RenderOTW *m_pRenderer;
	float m_fUVSkyMult;
	float m_fUVSunMult;
	int m_fSkyCurY;
	DWORD *m_pSkyTexture;
	Tpoint m_vSkyVerts[PHIMAX][THETAMAX];
	Tpoint m_vSkyNormals[PHIMAX][THETAMAX];
	ThreeDVertex m_vXFormedSkyVerts[PHIMAX][THETAMAX];
	BOOL m_bDoLightning;
	BOOL m_bIsLightning;
	Tpoint m_vLightningGroundPos;
	int	m_nOldCondition;
	int	m_nConditionCounter;
	int	m_nLastDay;
	int m_nLastMoonTime;
	int	m_nMinTemperature;
	int	m_nMidTemperature;
	int	m_nMaxTemperature;
	int	m_nMinWindSpeed;
	int	m_nMidWindSpeed;
	int	m_nMaxWindSpeed;
	int	m_nThreshWindSpeed;
	BOOL m_bGreenMode;
	BOOL m_bAboveLayer;
	BOOL m_bBelowLayer;
	BOOL m_bInsideLayer;
	BOOL m_bDidOnce;
	int m_nStartMS;
	int m_nOldTimeMS;
	int m_nIntervalMS;
	Tpoint m_vLightningPos;
	float m_fSunYaw;
	float m_fMoonYaw;
	float m_fSunPitch;
	float m_fSunTheta;
	float m_fMoonPitch;
	Tpoint m_vRain;
	float m_fSunAngle;
	float m_fSunMagnitude;
	Tpoint m_vLightVector;
	float m_fWeatherShiftX;
	float m_fWeatherShiftY;
	float m_fLightningStep;
	float m_fLightningRadius;
	int	m_nMoonPhase;
	Tpoint m_vSunCoord;
	Tpoint m_vMoonCoord;
	float m_fAmbient;
	float m_fDiffuse;
	float m_fSpecular;
	float m_fRMult;
	float m_fMMult;
	float m_fMieG;
	float m_fSunIntensity;
	float m_fGammaR;
	float m_fGammaG;
	float m_fGammaB;
	float m_fRGBExposure;
	float m_fTurbidity;
	float m_fSunLambda[3];
	float m_fSkyLambda[3];
	color m_colSunColor;
	color m_colBetaMie;
	color m_colBetaRayleigh;
	color m_colBetaExtinction;
	color m_fEarthColor;
	color m_colFogColor;
	float m_fLightMod;
	float m_fHorizonEffect;
	float m_fWeatherFactor;
	float m_fStarIntensity;
	float m_fSunAzimuth;
	float m_fSunAltitude;
	float m_fMoonAzimuth;
	float m_fMoonAltitude;
	BYTE m_byteMoonPhaseMask[8*64];
	BYTE m_byteCurrentMoonPhaseMask[8*64];
	int m_nTimeZoneMS;
	float m_fLatitude;
	float m_fLongitude;
	float m_fSinLatitude;
	float m_fCosLatitude;
	int	m_nYear;
	int	m_nMonth;
	int	m_nDay;
	int	m_nDayOfYear;
	float m_fUniversalTime;
	float m_fUniversalTimeDegree;
	float m_fDeltaJulian;
	float m_fCurrentJulian;
	float m_fJulian1980;
	float m_fJulian2000;
	float m_fLocalSiderialTime;
	Texture SkyTexture;
	Texture SunTexture;
	Texture FlareTexture;
	Texture	RainTexture;
	Texture	CirrusTexture;
	Texture	CirrcumTexture;
	Texture	CumulusTexture;
	Texture	OvercastTexture;
	Texture	LightningTexture;
	Texture	MoonTexture;
	Texture	GreenMoonTexture;
	Texture	OriginalMoonTexture;
};
*/
//extern Weather *TheWeather;

inline void RotatePoint(Tpoint *t, float p, float r, float y)
{
	Trotation tRot;
	float costha,sintha,cosphi,sinphi,cospsi,sinpsi;

	costha = Cos(p);
	sintha = Sin(p);
	cosphi = Cos(r);
	sinphi = Sin(r);
	cospsi = Cos(y);
	sinpsi = Sin(y);

	tRot.M11 = cospsi*costha;
	tRot.M21 = sinpsi*costha;
	tRot.M31 = -sintha;

	tRot.M12 = -sinpsi*cosphi+cospsi*sintha*sinphi;
	tRot.M22 = cospsi*cosphi+sinpsi*sintha*sinphi;
	tRot.M32 = costha*sinphi;

	tRot.M13 = sinpsi*sinphi+cospsi*sintha*cosphi;
	tRot.M23 = -cospsi*sinphi+sinpsi*sintha*cosphi;
	tRot.M33 = costha*cosphi;

 	register float s_x = tRot.M11*t->x+tRot.M12*t->y+tRot.M13*t->z;
 	register float s_y = tRot.M21*t->x+tRot.M22*t->y+tRot.M23*t->z;
 	register float s_z = tRot.M31*t->x+tRot.M32*t->y+tRot.M33*t->z;
 
 	t->x = s_x;
 	t->y = s_y;
 	t->z = s_z;
}

static float cloudPntList[NUM_3DCLOUD_POINTS][3] =
{
	-6040,	-540,	-19030,	
	-28430,	-1790,	28260,	
	36930,	3890,	50410,	
	2870,	-1700,	46810,	
	-58350,	-1700,	-13060,	
	31070,	3200,	-43320,	
	-97160,	-1700,	3950,	
	-75200,	-1500,	76520,	
	55790,	-5180,	-83100,	
	-51000,	-320,	-66850,
	62260,	-320,	61570,	
	-61710,	-3340,	32190,	
	-61850,	-51250,	54330,	
	-6040,	-51540,	-19030,	
	-21460,	-51970,	18240,	
	43400,	-52210,	24120,	
	-51700,	-52160,	19010,	
	-8570,	-51330,	-52390,	
	45500,	-53400,	-15510,	
	-61850,	-51250,	54330,
	54580,	-52700,	79020,	
	85960,	-52310,	64840,	
	-100490,-52500,	88140,	
	-82780,	-51690,	-96710,	
	-9370,	-51540,	410,	
	-21460,	-51850,	49070,	
	4040,	-52810,	69180,	
	-26080,	-52160,	19490,	
	2810,	-51330,	-34840,	
	-61850,-51670,	90860,
	54580,	-52700,	79020,	
	85960,	-52310,	64840,	
	-100490,-52500,	88140,	
	98460,	-52500,	-45730,	
	16100,	-51320,	-51620,	
	58220,	-52740,	-57520,	
	-9370,	-540,	410,	
	-1840,	-20870,	35430,	
	-41740,	18720,	77740,	
	9060,	-9650,	-74790,
};

// RV - I-Hawk - the older values before the clouds fix...

//static float cloudPntList[NUM_3DCLOUD_POINTS][3] =
//{
//	  1840,	-20870,	35430,	
//	  2310,	-51330,	-34840,	
//	  2870,	-1700,	46810,	
//	  4040,	-52810,	69180,	
//	  6040,	-540,	-19030,	
//	  6040,	-51540,	-19030,	
//	  8570,	-51330,	-52390,	
//	  9060,	-9650,	-74790,
//	  9370,	-540,	410,	
//	  9870,	-51540,	410,	
//	 16100,	-51320,	-51620,	
//	 21460,	-51850,	49070,	
//	 24460,	-51970,	18240,	
//	 26080,	-52160,	19490,	
//	 28430,	-1790,	28260,	
//	 31070,	3200,	-43320,	
//	 36930,	3890,	50410,	
//	 41740,	18720,	77740,	
//	 43400,	-52210,	24120,	
//	 45500,	-53400,	-15510,	
//	 48580,	-52700,	79020,	
//	 51000,	-320,	-66850,
//	 53700,	-52160,	19010,	
//	 54580,	-52700,	79020,	
//	 55790,	-5180,	-83100,	
//	 58220,	-52740,	-57520,	
//	 58350,	-1700,	-13060,	
//	 61710,	-3340,	32190,	
//	 62260,	-320,	61570,	
//	 64850,-51670,	90860,
//	 71850,	-51250,	54330,
//	 75200,	-1500,	76520,	
//	 75960,	-52310,	64840,	
//	 81850,	-51250,	54330,	
//	 82780,	-51690,	-96710,	
//	 85960,	-52310,	64840,	
//	 91160,	-1700,	3950,	
//	 98460,	-52500,	-45730,	
//   98490,-52500,	88140,	
//  100490, -52500,	88140,	
//};

static float lightningPosList[NUM_LIGHTNING_POINTS][2] =
{
	11,0,
	7,5,
	9,8,
	8,14,
	16,20,
	10,24,
	11,27,
	6,30,
	7,33,
	1,38,
	5,42,
	5,45,
	12,48,
	20,52,
	20,58,
	23,61,
	22,63,
	22,66,
	25,68,
	20,70,
	24,75,
	24,77,
	31,80,
	31,83,
	33,88,
	31,92,
	47,103,
	46,105,
	47,107,
	45,112,
	48,114,
	46,117,
	43,128
};

#endif // _REALWEATHER_H_