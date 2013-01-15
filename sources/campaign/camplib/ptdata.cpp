// PtData.cpp
// Functions using the point data
#include <conio.h>
#include "falclib.h"
#include "entity.h"
#include "ClassTbl.h"
#include "feature.h"
#include "f4find.h"
#include "ListAdt.h"
#include "Objectiv.h"
#include "PtData.h"
#include "Weather.h"

extern short NumPtHeaders;
extern short NumPts;

// ===================================
// Global Pt data functions
// ===================================

int GetTaxiPosition(int point, int rwindex)
{
	int count = 0;
	int pt = PtHeaderDataTable[rwindex].first;
	while(pt && pt != point){
		if(pt > point) break;	 // 24JAN04 - FRB - Cover case of a/c on parking spot (not TaxiPt)
		pt = GetNextTaxiPt(pt);
		count++;
	}
	return pt ?  count : 0;
}

int GetCritTaxiPt (int headerindex)
{
	int point = PtHeaderDataTable[headerindex].first;
	
	while(PtDataTable[point].type != CritTaxiPt)
	{
		if(PtDataTable[point].flags & PT_LAST)
			return point;

		point++;
	}
	return point;
}

int GetFirstPt (int headerindex)
{
	return PtHeaderDataTable[headerindex].first;
}

int GetNextPt (int ptindex)
{
	if ((ptindex < 0) || (ptindex >= NumPts))
		ptindex = 0;
	if (!(PtDataTable[ptindex].flags & PT_LAST)){
		return ptindex + 1;
	}
	return 0;
}

int GetNextTaxiPt (int ptindex)
{
	// FRB - CTD's here
	if ((ptindex < 0) || (ptindex >= NumPts))
		ptindex = 0;
	ptindex = GetNextPt(ptindex);
	while(ptindex && PtDataTable[ptindex].type != TaxiPt && PtDataTable[ptindex].type != CritTaxiPt){
		ptindex = GetNextPt(ptindex);
	}
	return ptindex;
}

int GetNextPtLoop (int ptindex)
{
	if ((ptindex < 0) || (ptindex >= NumPts))
		ptindex = 0;
	if (!(PtDataTable[ptindex].flags & PT_LAST)){
		return ptindex + 1;
	}
	return ptindex;
}

int GetNextPtCrit (int ptindex)
{
	if ((ptindex < 0) || (ptindex >= NumPts))
		ptindex = 0;
	if (PtDataTable[ptindex].type != CritTaxiPt && !(PtDataTable[ptindex].flags & PT_LAST))
		return ptindex + 1;
	return 0;
}

int GetPrevPt (int ptindex)
{
	if ((ptindex < 0) || (ptindex >= NumPts))
		ptindex = 0;
	if (!(PtDataTable[ptindex].flags & PT_FIRST))
		return ptindex - 1;
	return 0;
}

int GetPrevTaxiPt (int ptindex)
{
	if ((ptindex < 0) || (ptindex >= NumPts))
		ptindex = 0;
	ptindex = GetPrevPt(ptindex);
	while(ptindex && PtDataTable[ptindex].type != TaxiPt && PtDataTable[ptindex].type != CritTaxiPt)
		ptindex = GetPrevPt(ptindex);
	
	return ptindex;
}

int GetPrevPtLoop (int ptindex)
{
	if ((ptindex < 0) || (ptindex >= NumPts))
		ptindex = 0;
	if (!(PtDataTable[ptindex].flags & PT_FIRST))
		return ptindex - 1;
	return ptindex;
}

int GetPrevPtCrit (int ptindex)
{
	if ((ptindex < 0) || (ptindex >= NumPts))
		ptindex = 0;
	if (PtDataTable[ptindex].type != CritTaxiPt && !(PtDataTable[ptindex].flags & PT_FIRST))
		return ptindex - 1;
	return 0;
}

int GetQueue(int rwindex)
{
	return PtHeaderDataTable[rwindex].runwayNum;
}

void TranslatePointData (CampEntity e, int ptindex, float *x, float *y)
{
	if ((ptindex < 0) || (ptindex >= NumPts))
		ptindex = 0;

	if (e && e->IsObjective()){
		// KCK TODO: Rotate these points by objective's heading before translating
		// SCR 11/29/98  I don't think objectives HAVE headings, so this is correct.
		*x = e->XPos();
		*y = e->YPos();
		*x += PtDataTable[ptindex].yOffset;	 // KCK NOTE: axis' are reversed
		*y += PtDataTable[ptindex].xOffset;
	}
}

// This will return the lowest status of the features this header is dependent on
int CheckHeaderStatus (CampEntity e, int index)
{
	int		status = VIS_NORMAL,i=0,fs;

	while (status != VIS_DESTROYED && i < MAX_FEAT_DEPEND)
	{
		if (PtHeaderDataTable[index].features[i] < 255)
		{
			if (e && e->IsObjective())
			{
				fs = ((Objective)e)->GetFeatureStatus(PtHeaderDataTable[index].features[i]);
//				ShiAssert(((Objective)e)->GetFeatureValue(PtHeaderDataTable[index].features[i]) > 0);
			}
			else
				fs = VIS_NORMAL;
			if (fs > status)
				status = fs;
		}
		i++;
	}
	return status;
}

// JPO - support for parking places - not sure if that was what these were for originally
int GetFirstParkPt (int headerindex)
{
	int pt = PtHeaderDataTable[headerindex].first;
	while(pt)
	{
		switch (PtDataTable[pt].type) {
		case SmallParkPt:
		case LargeParkPt:
			return pt; // found a parking space
		}
		if (PtDataTable[pt].flags & PT_LAST)
			return 0; // examined all
		pt ++;	// FRB - Should pt be incremented????	I added pt++;	 fn() not used :^(
	}
	return 0;
}

int GetNextParkPt (int pt)
{
	if (PtDataTable[pt].flags & PT_LAST)
		return 0; // stop
	pt ++;
	while(pt)
	{
		switch (PtDataTable[pt].type) {
		case SmallParkPt:
		case LargeParkPt:
			return pt; // found a parking space
		}
		if (PtDataTable[pt].flags & PT_LAST)
			return 0; // examined all
		pt ++;
	}
	return 0;
}

int GetPrevParkPt (int pt)
{
	if (PtDataTable[pt].flags & PT_FIRST)
		return 0; // stop
	pt --;
	while(pt > 0)
	{
		switch (PtDataTable[pt].type) {
		case SmallParkPt:
		case LargeParkPt:
			return pt; // found a parking space
		}
		if (PtDataTable[pt].flags & PT_FIRST)
			return 0; // examined all
		pt --;
	}
	return 0;
}

int GetNextParkTypePt (int pt, int type)
{
	if (PtDataTable[pt].flags & PT_LAST)
		return 0; // stop
	pt ++;
	while(pt)
	{
		if (PtDataTable[pt].type == type){
			return pt; // found a parking space
		}
		if (PtDataTable[pt].flags & PT_LAST){
			return 0; // examined all
		}
		pt ++;
	}
	return 0;
}

int GetPrevParkTypePt (int pt, int type)
{
	if (PtDataTable[pt].flags & PT_FIRST)
		return 0; // stop
	pt --;
	while(pt > 0)
	{
		if(PtDataTable[pt].type == type)
			return pt; // found a parking space
		if (PtDataTable[pt].flags & PT_FIRST)
			return 0; // examined all
		pt --;
	}
	return 0;
}