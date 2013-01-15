// =========================================
// Global Campaign Definitions and Variables
// =========================================

#ifndef CMPGLOBL
#define CMPGLOBL

//#include "shi/shi.h"
#include "sim/include/stdhdr.h"
#include "Camplib.h"
#include "Graphics/Include/Ttypes.h"

// ----------------
// Type Definitions
// ----------------

// sfr: number or rows or columns of the trees holding grid units (campaign)
#define TREE_RES 100

#define GRID_SIZE_FT    FEET_PER_KM		// Grid size, in feet (standard sim unit)
#define GRID_SIZE_KM	1.0F			// Grid size, in km (standard campaign unit)

#define	DEG_TO_RADIANS	0.017453F		// PI / 180
#define RADIANS_TO_DEG	57.29578F		// 180 / PI

#define HALF_CHANCE		16000			// Half of RAND_MAX

#define TOD_SUNUP		300				// Sun up, in minutes since midnight
#define TOD_SUNDOWN		1260			// Sun down, in minutes since midnight
#define TOD_NIGHT		1
#define TOD_DAWNDUSK	2
#define TOD_DAY			3

#define TYPE_NE			0				// Equivalency defines.. See FORTRAN.. ;-)
#define TYPE_LT			1
#define TYPE_LE			2
#define TYPE_EQ			3
#define TYPE_GE			4
#define TYPE_GT			5

#define NOT_DETECTED	0
#define DETECTED_VISUAL	1
#define DETECTED_RADAR	2

extern CampaignTime ReconLossTime[MOVEMENT_TYPES];

typedef enum	{ StatuteMiles,NauticalMiles,Kilometers } DistanceUnitType;
				  
// Define these as needed depending on compiler and machine
typedef short int       twobyte;
typedef long  int       fourbyte;
typedef double          eightbyte; 

typedef enum	{ GroundAltitude,LowAltitude,MediumAltitude,HighAltitude,VeryHighAltitude } AltitudeLevelType;
#define	ALT_LEVELS		5

typedef struct	{	unsigned char EastOfGreenwich;    // East Longitude is "Negative"
					unsigned char SouthOfEquator;     // South Latitude is "Negative"
					char    DegreesOfLatitude;  // 0 to 90
					char    MinutesOfLatitude;  // 0 to 60
					char    SecondsOfLatitude;  // 0 to 60
					char    DegreesOfLongitude; // 0 to 180
					char    MinutesOfLongitude; // 0 to 60
					char    SecondsOfLongitude; // 0 to 60
				} LatLong;

typedef char CampaignSaveKey;

typedef uchar Percentage;
typedef uchar Control;
typedef uchar Team;
typedef uchar Value;
typedef uchar UnitType;
typedef uchar UnitSize;
typedef uchar ObjectiveType;
typedef uchar CampaignOrders;

typedef enum {	NoRelations,
				Allied,
				Friendly,
				Neutral,
				Hostile,
				War } RelType;

#define MOVE_GROUND(X) ((X)==Foot || (X)==Wheeled || (X)==Tracked)
#define MOVE_AIR(X)    ((X)==Air || (X)==LowAir)
#define MOVE_NAVAL(X)  ((X)==Naval)
#define MOVE_NONE(X)   ((X)==NoMove)

#ifndef MAX
#  define MAX(a,b)                      ((a)>(b) ? (a) : (b))
#endif
#ifndef MIN
#  define MIN(a,b)                      ((a)<(b) ? (a) : (b))
#endif

#define North			0
#define NorthEast		1
#define East			2
#define SouthEast		3                                      
#define South			4
#define SouthWest		5
#define West			6
#define NorthWest		7
#define Here			8

typedef uchar CampaignHeading;
              
typedef char PriorityLevel;

typedef enum {	NullStatus,
				Operational,
				Damaged,
				Destroyed} ObjectiveStatus;

typedef enum {	OnGround,
				LowAlt,
				MediumAlt,
				HighAlt } AltitudeType;

typedef enum {	Flat,
				Rough,
				Hills,
				Mountains } ReliefType;
#define RELIEF_TYPES    4

typedef enum {	Water,                           // Cover types
				Bog,										
				Barren,
				Plain,
				Brush,
				LightForest,
				HeavyForest,
				Urban } CoverType;
#define COVER_TYPES     8   

#include "CampCell.h"
#include "CampTerr.h"

#endif
