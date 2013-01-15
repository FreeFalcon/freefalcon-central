#ifndef _RADARDATA_H_
#define _RADARDATA_H_

#pragma pack(push, 1)
enum { HIGH_ALT_LETHALITY = 0, LOW_ALT_LETHALITY, NUM_ALT_LETHALITY };
typedef struct RadarDataType
{
	int			RWRsound;						// Which sound plays in the RWR
	short			RWRsymbol;						// Which symbol shows up on the RWR
	short			RDRDataInd;		// Index into radar data files
	float		Lethality[NUM_ALT_LETHALITY];	// Lethality against low altitude targets
	float		NominalRange;					// Detection range against F16 sized target
	float		BeamHalfAngle;					// radians (degrees in file)
	float		ScanHalfAngle;					// radians (degrees in file)
	float		SweepRate;						// radians/sec (degrees in file)
	unsigned	CoastTime;						// ms to hold lock on faded target (seconds in file)
	float		LookDownPenalty;				// degrades SN ratio
	float		JammingPenalty;					// degrades SN ratio
	float		NotchPenalty;					// degrades SN ratio
	float		NotchSpeed;						// ft/sec (kts in file)
	float		ChaffChance;					// Base probability a bundle of chaff will decoy this radar
	short		flag;							// 0x01 = NCTR capable
} RadarDataType;
#pragma pack(pop)

#define RAD_NCTR 0x01
#define RAD_TWS 0x02
#define RAD_TFR 0x04
#define RAD_AARADAR 0x08
#define RAD_AGRADAR 0x10

struct RadarDataSet { // stuff read from the .dat file
	char Indx; // name of the radar
    int prf; // pulse repetition factor
    int TimeToLock ; // time in miliseconds to acuire a lock 
    int MaxTwstargets ; // max number of targets displayed/tracked in tws mode
    int Timetosearch1 ; // battalion radar timers (when to go into this mode) in ms
    int Timetosearch2 ; // 
    int Timetosearch3 ; // 
    int Timetoacuire ; // 
    int Timetoguide ; // 
    int Timetocoast ; // 
    int Rangetosearch1 ; // battalion radar timers (when to go into this mode) in ms
    int Rangetosearch2 ; // 
    int Rangetosearch3 ; // 
    int Rangetoacuire ; // 
    int Rangetoguide ; // 
    int Sweeptimesearch1 ; // time in ms between radar beam hits
    int Sweeptimesearch2 ; // 
    int Sweeptimesearch3 ; // 
    int Sweeptimeacuire ; // 
    int Sweeptimeguide ; // 
    int Sweeptimecoast ; // 
    int Timeskillfactor ; // how much does the skill effect the timers
    int Rwrsoundsearch1 ; // rwr sounds
    int Rwrsoundsearch2 ; // 
    int Rwrsoundsearch3 ; // 
    int Rwrsoundacuire; // 
    int Rwrsoundguide ; // 
    int Rwrsymbolsearch1 ; // rwr symbols
    int Rwrsymbolsearch2 ; // 
    int Rwrsymbolsearch3 ; // 
    int Rwrsymbolacuire; // 
    int Rwrsymbolguide ; // 
	int AirFireRate;// minimun ms between launces
	int Maxmissilesintheair;// this radar can maximum support this number of missile engagements
	int Elevationbumpamounta;
	int Elevationbumpamountb;
	int AverageSpeed;
	float MaxAngleDiffTws;
	float MaxRangeDiffTws;
	float MaxAngleDiffSam;
	float MaxRangeDiffSam;
	float MaxNctrRange;
	float NctrDelta;
// 2002-02-26 MN some SAM stuff
	float MinEngagementAlt;
	float MinEngagementRange;
};

extern RadarDataType*			RadarDataTable;
extern short NumRadarEntries;
extern RadarDataSet *radarDatFileTable;
extern short NumRadarDatFileTable;

extern void ReadAllRadarData();
#endif
