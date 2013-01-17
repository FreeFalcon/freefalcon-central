// ********************************************************
//
// feature.h
//
// ********************************************************

#ifndef PTDATA_H
#define PTDATA_H

#include "Entity.h"

// ==================================
// Point Flags
// ==================================

#define PT_FIRST		0x01					// Flag set if first in pt list
#define PT_LAST			0x02					// Flag set if last in pt list
#define PT_OCCUPIED		0x04				// 02JAN04 - FRB - Flag set if last Parking spot occupied

#define USAGE_LANDING	0						// Passed to FindRunways() as a usage type
#define USAGE_TAKEOFF	1

// ==================================
// Error codes
// ==================================

#define DPT_ERROR_NOT_READY		-1
#define DPT_ERROR_CANT_PLACE	-2

// ==================================
// Special return values
// ==================================

#define DPT_ONBOARD_CARRIER		32000

// ==================================
// Point Types
// ==================================

enum PointTypes {	NoPt			= 0,
					RunwayPt,
					TakeoffPt,
					TaxiPt,
					SAMPt,
					ArtilleryPt,
					AAAPt,
					RadarPt,
					RunwayDimPt,
					SupportPt,
					StaticRadarPt,
					SmallParkPt,
					LargeParkPt,
					SmallDockPt,
					LargeDockPt,
					TakeRunwayPt,
					HelicopterPt,
					FollowMePt,
					TrackPt,
					CritTaxiPt
				};

enum PointListTypes {
					NoList				= 0,
					RunwayListType		= 1,
					SAMListType			= 4,
					ArtListType			= 5,
					AAAListType			= 6,
					RnwyDimListType		= 8,
					StaticRadarListType	= 10,
					ParkListType		= 11,
					RnwyListLtType		= 12,
					RnwyListRtType		= 13,
					HeliListType		= 14,
					FollowListType		= 15,
					DockListType		= 16,
					TrackListType		= 17,
				};

// ==================================
// Point functions
// ==================================

#ifndef CLASSMKR

int GetTaxiPosition(int point, int rwindex);

extern int GetCritTaxiPt (int headerindex);

extern int GetFirstPt (int headerindex);

extern int GetNextPt (int ptindex);

extern int GetNextTaxiPt (int ptindex);

extern int GetNextPtLoop (int ptindex);

extern int GetNextPtCrit (int ptindex);

extern int GetPrevPt (int ptindex);

extern int GetPrevTaxiPt (int ptindex);

extern int GetPrevPtLoop (int ptindex);

extern int GetPrevPtCrit (int ptindex);

extern void TranslatePointData (CampEntity e, int ptindex, float *x, float *y);

extern int FindRunways (CampEntity airbase, int usage, int *rw1, int *rw2, int findall);

extern int CheckHeaderStatus (CampEntity e, int index);

int GetQueue(int rwindex);

int GetFirstParkPt (int headerindex);
int GetNextParkPt (int pt);
int GetPrevParkPt (int pt);
int GetNextParkTypePt (int pt, int type);
int GetPrevParkTypePt (int pt, int type);


#endif

#endif
