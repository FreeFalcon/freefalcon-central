#ifndef CMPRADAR_H
#define CMPRADAR_H

#include "SIM/INCLUDE/phyconst.h"

// KCK: This class in placeholder until I get around to actually coding this stuff

#define	NUM_RADAR_ARCS					8			// How many arcs each radar data will store
#define ALT_FOR_RANGE_DETERMINATION		2500.0F		// when we return range, it'll be how far we can
													// see something at this altitude
#define MINIMUM_RADAR_RATIO				0.022F		// Minimum ratio (about 1 deg angle)

class RadarRangeClass
	{
	public:
		float		detect_ratio[NUM_RADAR_ARCS];	// tan(detection_angle)- basically, ratio of 
													// altitude to distance above which we can see

	public:
		int GetNumberOfArcs (void)							{ return NUM_RADAR_ARCS; };
		float GetArcRatio (int anum)						{ return detect_ratio[anum]; };
// 2001-03-14 MODIFIED BY S.G. detect_ratio[anum] CAN BE ZERO. NEED TO CHECK FOR THAT FIRST
//		float GetArcRange (int anum)						{ return ALT_FOR_RANGE_DETERMINATION/detect_ratio[anum]; };
		float GetArcRange (int anum)						{ return detect_ratio[anum] != 0.0f ? ALT_FOR_RANGE_DETERMINATION/detect_ratio[anum] : 0.0f; };
		int CanDetect (float dx, float dy, float dz);
		float GetRadarRange (float dx, float dy, float dz);
		void GetArcAngle (int anum, float* a1, float *a2)	{ *a1 = anum * (360/NUM_RADAR_ARCS) * DTR;
															  *a2 = (anum+1) * (360/NUM_RADAR_ARCS) * DTR; };

		void SetArcRatio (int anum, float ratio)			{ detect_ratio[anum] = ratio; };
	};

#endif