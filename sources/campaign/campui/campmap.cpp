// 
// Map.cpp deals with the various modes of coloring the small theater map
//

#include <stdio.h>
#include <tchar.h>
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Find.h"
#include "CampMap.h"
#include "Objectiv.h"
#include "CampList.h"
#include "Campaign.h"
#include "Team.h"
#include "CmpClass.h"
#include "FalcSess.h"
#include "CmpRadar.h"

extern void UI_UpdateOccupationMap();

#define MAP_RADIUS				20

// 2001-03-14 MODIFIED BY S.G. SO IT USES THE SAME VALUE AS GetArcRange
//int ALT_TO_BUILD_RANGES_TO = 5000;		// What altitude we should draw our low alt detection ranges to
int ALT_TO_BUILD_RANGES_TO = 2500;		// What altitude we should draw our low alt detection ranges to

// =================================
// Prototypes
// =================================

int AddToThreatMap (CampEntity e, uchar* map_data, int who);
int AddToDetectionMap (CampEntity e, uchar* map_data, int who);

// =================================
// A few globals
// =================================

int MRX;
int MRY;
int PMRX;
int PMRY;
int MAXOI;

// ==============================================
// Map coloring stuff
// ==============================================

uchar* MakeCampMap (int type, uchar* map_data, int csize)
{
	GridIndex	x,y,rx,ry;
	int			i,hi,size,team;
	Objective	o;

	team = FalconLocalSession->GetTeam();
	if (team > NUM_TEAMS)
		team = 0;

	switch (type){
		case MAP_SAMCOVERAGE:
//			MRX = Map_Max_X/MAP_RATIO;
//			MRY = Map_Max_Y/MAP_RATIO;
			size = sizeof(uchar)*MRX*MRY;
			break;
		case MAP_RADARCOVERAGE:
//			MRX = Map_Max_X/MAP_RATIO;
//			MRY = Map_Max_Y/MAP_RATIO;
			size = sizeof(uchar)*MRX*MRY;
			break;
		case MAP_PAK:
		case MAP_PAK_BUILD:
//			PMRX = Map_Max_X/PAK_MAP_RATIO;
//			PMRY = Map_Max_Y/PAK_MAP_RATIO;
			size = sizeof(uchar)*PMRX*PMRY;
			break;
		case MAP_OWNERSHIP:
		default:
//			MRX = Map_Max_X/MAP_RATIO;
//			MRY = Map_Max_Y/MAP_RATIO;
			size = sizeof(uchar)*MRX*MRY/2;
			break;
	}
	if (size != csize || !map_data)
	{
		// better resize it
		CampEnterCriticalSection();
		if (map_data)
			delete [] map_data;
		map_data = new unsigned char[size];
		CampLeaveCriticalSection();
	}
	memset(map_data,0,size);

	switch (type)
	{
		case MAP_RADARCOVERAGE:
		{
			VuListIterator	uit(EmitterList);
			CampEntity		e;
			e = (CampEntity) uit.GetFirst();
			while (e){
				AddToDetectionMap (e, map_data, team);
				e = (CampEntity) uit.GetNext();
			}
		}
		break;
		case MAP_SAMCOVERAGE:
		{
			VuListIterator	uit(AirDefenseList);
			Unit e;
			e = (Unit) uit.GetFirst();
			while (e){
				if (!e->Moving()){
					AddToThreatMap (e, map_data, team);
				}
				e = (Unit) uit.GetNext();
			}
		}
		break;
		case MAP_PAK:
		{
			FILE		*fp = OpenCampFile(TheCampaign.TheaterName,"pak","rb");
			int got = fread(map_data,1, size,fp);
			ShiAssert(got == size);
			fclose(fp);
		}
		break;
		case MAP_PAK_BUILD:
		{
			VU_ID			pakTable[50];
			int				p,done,own;

			// Build our table
			for (p=0;p<50;p++)
				pakTable[p] = FalconNullId;
			p = 1;
			{
				VuListIterator	poit(POList);
				o = (Objective) poit.GetFirst();
				while (o){
					pakTable[p] = o->Id();
					p++;
					o = (Objective) poit.GetNext();
				}
			}
				
			for (x=0; x<PMRX-1; x++){
				for (y=0; y<PMRY-1; y++){
					i = ((PMRY-1) * PMRX) - y*PMRX + x;
					rx = x*PAK_MAP_RATIO;
					ry = y*PAK_MAP_RATIO;
					own = GetOwner(TheCampaign.CampMapData, rx, ry);
					// Let the ownership map decide if we're over water or not
					//if (GetCover(rx,ry) != Water || GetCover(rx+PAK_MAP_RATIO-1,ry) != Water || 
					// GetCover(rx,ry+PAK_MAP_RATIO-1) != Water)
					if (own){
						float	last = -1.0F;
						o = FindNearestObjective(POList, rx, ry,&last);
						while (o && o->GetTeam() != own){
							o = FindNearestObjective(POList, rx, ry, &last);
						}
						if (o){
							for (p=1,done=0;p<50&&!done&&o;p++){
								if (o->Id() == pakTable[p]){
									map_data[i] = p;
									done = 1;
								}
							}
							ShiAssert(done);
						}
						else
							map_data[i] = 255;
					}
				}
			}
		}
		break;
		case MAP_OWNERSHIP:
		default:
			for (x=0; x<MRX; x++){
				for (y=0; y<MRY; y++){
					i = y*MRX + x;
					hi = 4*(i%2);
					rx = x*MAP_RATIO;
					ry = y*MAP_RATIO;
					if (
						GetCover(rx,ry) != Water || 
						GetCover(rx+MAP_RATIO-1,ry) != Water || 
						GetCover(rx,ry+MAP_RATIO-1) != Water
					){
						// KCK: Search a small area first, then if I don't find something, search a larger area
						o = FindNearestObjective(rx, ry, NULL, 10);
						if (!o){
							o = FindNearestObjective(rx, ry, NULL, 80);
						}
						
						if (o){
							map_data[i/2] |= o->GetTeam() << hi;
						}
						else {
							map_data[i/2] |= 0xF << hi;
						}
					}
				}
			}
			UI_UpdateOccupationMap();
			break;
		}
	return map_data;
	}

// This will update a 20 km radius area around cx,cy
uchar* UpdateCampMap (int type, uchar* map_data, GridIndex cx, GridIndex cy)
	{
	GridIndex	x,y,rx,ry,fx,fy,lx,ly;
	int			i,hi;
	Objective	o;

	if (!map_data)
		return NULL;

	fx = cx - MAP_RADIUS;
	lx = cx + MAP_RADIUS;
	fy = cy - MAP_RADIUS;
	ly = cy + MAP_RADIUS;
	if (fx < 0)
		fx = 0;
	if (fy < 0)
		fy = 0;
	if (lx > Map_Max_X - 1)
		lx = Map_Max_X - 1;
	if (ly > Map_Max_Y - 1)
		ly = Map_Max_Y - 1;
	fx /= MAP_RATIO;
	fy /= MAP_RATIO;
	lx /= MAP_RATIO;
	ly /= MAP_RATIO;

	switch (type)
		{
		case MAP_RADARCOVERAGE:
		case MAP_SAMCOVERAGE:
			break;
		case MAP_OWNERSHIP:
		default:
			for (x=fx; x<lx; x++){
				for (y=fy; y<ly; y++){
					i = y*MRX + x;
					hi = 4*(i%2);
					rx = x*MAP_RATIO;
					ry = y*MAP_RATIO;
					if (
						GetCover(rx,ry) != Water || 
						GetCover(rx+MAP_RATIO-1,ry) != Water || 
						GetCover(rx,ry+MAP_RATIO-1) != Water
					){
						// KCK: Searh a small area first, then if I don't find something, search a larger area
						o = FindNearestObjective(rx, ry, NULL, 10);
						if (!o){
							o = FindNearestObjective(rx, ry, NULL, 80);
						}
						if (o){
							map_data[i/2] |= o->GetTeam() << hi;
						}
						else {
							map_data[i/2] |= 0xF << hi;
						}
					}
				}
			}
			// KCK: Robin or Peter - this will cause deadlock - Entering a critical section from here is a no-no
			// Either post a message or set a dirty flag..
			// UI_UpdateOccupationMap();
			break;
		}
	return map_data;
	}

uchar GetOwner (uchar* map_data, GridIndex x, GridIndex y)
	{
	int		i,hi;

	if (!map_data)
		return 0;
	i = (y/MAP_RATIO)*MRX + (x/MAP_RATIO);
	if (i < 0 || i > MAXOI)
		return 0;
	hi = 4 * (i & 1);
	return ((map_data[i/2] >> hi) & 0x0F);
	}

int FriendlyTerritory (GridIndex x, GridIndex y, int team)
	{
	if (GetOwner(TheCampaign.CampMapData,x,y) == team)
		return 1;
	return 0;
	}

int GetAproxDetection (Team who, GridIndex x, GridIndex y)
	{
	int		i,ix;

	// Find our indexes
	i = (y/MAP_RATIO)*MRX + (x/MAP_RATIO);
	if (i < 0 || i > TheCampaign.RadarMapSize)
		return 0;				// Off the map
	if (who == FalconLocalSession->GetTeam())
		ix = 4;
	else
		ix = 0;

	return (TheCampaign.RadarMapData[i] >> ix) & 0x03;
	}

int GetAproxThreat (Team who, GridIndex x, GridIndex y)
	{
	int		i,ix;

	// Find our indexes
	i = (y/MAP_RATIO)*MRX + (x/MAP_RATIO);
	if (i < 0 || i > TheCampaign.RadarMapSize)
		return 0;				// Off the map
	if (who == FalconLocalSession->GetTeam())
		ix = 4;
	else
		ix = 0;
	
	return (TheCampaign.SamMapData[i] >> ix) & 0x03;
	}


void FreeCampMap (uchar *map_data)
	{
	if (map_data)
		delete [] map_data;
	}

// =================================
// Map building stuff
// =================================

int AddToThreatMap (CampEntity e, uchar* map_data, int who)
	{
	GridIndex   x,y,X,Y;
	int			fx,lx,fy,ly,bd,li,hi,i,c;
	float		d,ld,hd;

	e->GetLocation(&X,&Y);
	X /= MAP_RATIO;
	Y /= MAP_RATIO;
	ld = (float) e->GetWeaponRange(LowAir)/MAP_RATIO;
	hd = (float) e->GetWeaponRange(Air)/MAP_RATIO;
	bd = MAX(FloatToInt32(hd),FloatToInt32(ld));
	fx = MAX(X - bd - 1, 0);
	lx = MIN(X + bd + 1, MRX-1);
	fy = MAX(Y - bd - 1, 0);
	ly = MIN(Y + bd + 1, MRY-1);
	if (GetRoE(e->GetTeam(),who,ROE_AIR_ENGAGE) == ROE_ALLOWED)
		{
		li = 4;
		hi = 6;
		}
	else
		{
		li = 0;
		hi = 2;
		}
	for (x = fx; x <= lx; x++)
		{
		for (y = fy; y <= ly; y++)
			{
			i = y*MRX + x;
			d = Distance(x,y,X,Y) - 1.0F;
			c = (map_data[i] >> li) & 0x03;
			if (ld >= d && c < 3 && e->GetAproxHitChance(LowAir,FloatToInt32(d*MAP_RATIO)))
				{
				map_data[i] ^= (c << li);
				map_data[i] |= ((c+1) << li);
				}
			c = (map_data[i] >> hi) & 0x03;
			if (hd >= d && c < 3 && e->GetAproxHitChance(Air,FloatToInt32(d*MAP_RATIO)))
				{
				map_data[i] ^= (c << hi);
				map_data[i] |= ((c+1) << hi);
				}
			}
		}
	return 1;
	}

int AddToDetectionMap (CampEntity e, uchar* map_data, int who)
	{
	GridIndex   x,y,X,Y;
	int			fx,lx,fy,ly,li,hi,i,c,oct,bdi;
	float		d,hd,bd,ld[NUM_RADAR_ARCS] /* 2001-03-13 S.G. */, ld0;

	if (!e->GetNumberOfArcs())
		return 0;

	bd = 0.0F;
	e->GetLocation(&X,&Y);
	X /= MAP_RATIO;
	Y /= MAP_RATIO;

// 2001-03-13 ADDED BY S.G. I NEED THE DETECTION RANGE FROM THE DATA FILE AS WELL, JUST LIKE THE THREAT MAP DOES
	ld0 = (float) e->GetDetectionRange(LowAir)/MAP_RATIO;
// END OF ADDED SECTION

	for (i=0; i<NUM_RADAR_ARCS; i++)
		{
// 2001-03-09 MODIFIEDED BY S.G. e->GetArcRatio(i) CAN BE ZERO! IF THAT HAPPENS, The for (x=... LOOP BELOW IS SQUIPPED SO HIGH ALTITUDE IS NOT MAPPED!
//		ld[i] = (float) (((ALT_TO_BUILD_RANGES_TO / e->GetArcRatio(i)) * FT_TO_KM)/MAP_RATIO);
		if (float arcRatio = e->GetArcRatio(i)) {
			ld[i] = (float) (((ALT_TO_BUILD_RANGES_TO / arcRatio) * FT_TO_KM)/MAP_RATIO);
			if (ld0 < ld[i])
				ld[i] = ld0;
		}
		else
			ld[i] = ld0;
// END OF MODIFIED SECTION
		if (ld[i] > bd)
			bd = ld[i];
		}
	hd = (float) e->GetDetectionRange(Air)/MAP_RATIO;
	if (hd > bd)
		bd = hd;
//	bd = MAX(FloatToInt32(hd),FloatToInt32(ld));
	bdi = FloatToInt32(bd);
	fx = MAX(X - bdi - 1, 0);
	lx = MIN(X + bdi + 1, MRX-1);
	fy = MAX(Y - bdi - 1, 0);
	ly = MIN(Y + bdi + 1, MRY-1);
	if (GetRoE(e->GetTeam(),who,ROE_AIR_ENGAGE) == ROE_ALLOWED)
		{
		li = 4;
		hi = 6;
		}
	else
		{
		li = 0;
		hi = 2;
		}
	for (x = fx; x <= lx; x++)
		{
		for (y = fy; y <= ly; y++)
			{
			i = y*MRX + x;
			d = Distance(x,y,X,Y) - 1.0F;
			c = (map_data[i] >> li) & 0x03;
			oct = OctantTo(X,Y,x,y);
			if (ld[oct] >= d && c < 3)
				{
				map_data[i] ^= (c << li);
				map_data[i] |= ((c+1) << li);
				}
			c = (map_data[i] >> hi) & 0x03;
			if (hd >= d && c < 3)
				{
				map_data[i] ^= (c << hi);
				map_data[i] |= ((c+1) << hi);
				}
			}
		}
	return 1;
	}
