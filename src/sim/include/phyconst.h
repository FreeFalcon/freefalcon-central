#ifndef _PHYCONST_H
#define _PHYCONST_H

/*--------------------*/
/* Physical Constants */
/*--------------------*/
#define RTD                     57.2957795F
#define DTR                     0.01745329F
#define VIS_RANGE               50000.0F
#define BIGGEST_RANDOM_NUMBER   32767.0F
#define BOLTZ                   1.38E-23
#ifndef PI
#define PI                      3.141592654F
#endif
#define HALF_PI					1.570796326795F
#define FOUR_PI_CUBED           1984.402F
#define FT_TO_METERS            0.30488F
#define FT_TO_NM                0.0001646F
#define FT_TO_KM				0.0003048F
#define NM_TO_FT                6076.211F
#define KM_TO_FT				FEET_PER_KM       /* This define is also in constant.h */
#define NM_TO_KM				1.85224731f
#define KM_TO_NM				0.53988471f
#define FIVE_G_TURN_RATE        15.9F
#define LIGHTSPEED              983319256.3F      /* feet per sec */
#define TASL                    518.7F
#define PASL                    2116.22F
#define RHOASL                  0.0023769F
#define AASL                    1116.44F
#define AASLK                   661.48F
#define GRAVITY                 32.177F
#define FTPSEC_TO_KNOTS         0.592474F
#define KNOTS_TO_FTPSEC         1.687836F
#define MILS_TO_DEGREES         0.057296F
#define DTMR                    17.45F
#define MRTD                    0.057296F
#define RTMR                    1000.0F
#define MRTR                    0.001F
#define KPH_TO_FPS              0.9111053F
#define EARTH_RADIUS_NM			  3443.92228F	//Mean Equatorial Radius
#define EARTH_RADIUS_FT			  2.09257E7F	//Mean Equatorial Radius
#define NM_PER_MINUTE			  1.00018F		//Nautical Mile per Minute of Latitude (Or Longitude at Equator)
#define MINUTE_PER_NM			  0.99820F		//Minutes of Latitude (or Longitude at Equator) per NM
#define FT_PER_MINUTE			  6087.03141F	//Feet per Minute of Latitude (Or Longitude at Equator)							
#define MINUTE_PER_FT			  1.64283E-4F	//Minutdes of Latitude (or Longitude at Equator) per foot
#define FT_PER_DEGREE			  FT_PER_MINUTE * 60.0F

#define DEG_TO_MIN				  60
#define MIN_TO_DEG				  0.01666666F

#define MIN_TO_SEC				  DEG_TO_MIN
#define SEC_TO_MIN				  MIN_TO_DEG

#define DEG_TO_SEC				  3600
#define SEC_TO_DEG				  2.7777777E-4F

#define SEC_TO_MSEC             1000
#define MSEC_TO_SEC             0.001f
#ifndef TRUE
#define TRUE   (1)
#endif

#ifndef FALSE
#define FALSE  (0)
#endif

#endif
