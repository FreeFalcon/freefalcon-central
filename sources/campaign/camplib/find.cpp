#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include "campmap.h"
#include "CmpGlobl.h"
#include "ListADT.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Entity.h"
#include "ASearch.h"
#include "Campaign.h"
#include "Objectiv.h"
#include "Unit.h"
#include "Update.h"
#include "Find.h"
#include "CampList.h"
#include "path.h"
#include "NoFly.h"
#include "AIInput.h"
#include "GndUnit.h"
#include "CmpClass.h"
#include "FalcSess.h"
#include "classtbl.h"
#include "Debuggr.h"
#include "uiwin.h"
#include "atcbrain.h"
#include "Flight.h"

// =====================================
// Campaign - Globals defines
// =====================================

#define MAX_SCORE       30
// For Testing

extern char           CellSize;
extern short int      OX,OY,FX,FY,LX,LY;
extern void ShowWP (MapData md, GridIndex X, GridIndex Y, int color);
extern void ShowRange (MapData md, GridIndex X, GridIndex Y, int range, int color);

extern int MRX;
extern int MRY;

// =========
// Data
// =========

float OffsetToMiddle = GRID_SIZE_FT/2.0F;

// This stuff is used to convert from altitude to altitude level and vice-versa,
// as well as a way to randomize altitudes within a level reasonably
int MaxAltAtLevel[ALT_LEVELS] = { 99, 4999, 19999, 39999, 99999 };
int MinAltAtLevel[ALT_LEVELS] = { 0, 100, 5000, 20000, 40000 };
int LevelIncrement[ALT_LEVELS] = { 0, 0, 1000, 2000, 2500 };
int IncrementMax[ALT_LEVELS] = { 0, 1, 8, 8, 8 };

extern costtype GetObjectiveMovementCost (Objective o, Objective t, int neighbor, MoveType type, Team team, int flags);

// =================================
// Globals 
// =================================

// uchar ThreatSearch[MAX_CAMP_ENTITIES];			// Search data

// =====================================
// Global functions
// =====================================

// Returns distance in kilometers
float Distance(GridIndex ox, GridIndex oy, GridIndex dx, GridIndex dy){
	return (float)sqrt((float)((ox-dx)*(ox-dx) + (oy-dy)*(oy-dy))) * GRID_SIZE_KM;
}

int DistSqu (GridIndex ox, GridIndex oy, GridIndex dx, GridIndex dy){
	return ((ox-dx)*(ox-dx) + (oy-dy)*(oy-dy));
}

// returns distance in feet
float Distance (float ox, float oy, float dx, float dy){
	return (float)sqrt(((ox-dx)*(ox-dx) + (oy-dy)*(oy-dy)));
}

float DistSqu(float ox, float oy, float dx, float dy){
	return (float)((ox-dx)*(ox-dx) + (oy-dy)*(oy-dy));
}

float DistanceToFront(GridIndex x, GridIndex y){
	float       d,lowest=999.0F;
	GridIndex   fx,fy;
	Objective   f;
	VuListIterator	myit(FrontList);
	f = GetFirstObjective(&myit);
	while (f != NULL){
		// RV - Biker - This is a hack because JimG did introduce 
		if (f->GetType() != TYPE_BORDER || f->GetSType() != 2) {
		f->GetLocation(&fx, &fy);
		d = Distance(x, y, fx, fy);
		if (d < lowest){
			lowest = d;
		}
		}
		f = GetNextObjective(&myit);
	}
	return lowest;
}

float DirectionToFront(GridIndex x, GridIndex y){
	float       d,r,lowest=999.0F;
	GridIndex   fx,fy;
	Objective   f,n;

	r = -1.0F;
	{
		VuListIterator	myit(FrontList);
		f = GetFirstObjective(&myit);
		while (f != NULL){
			// RV - Biker - This is a hack because JimG did introduce 
			if (f->GetType() != TYPE_BORDER || f->GetSType() != 2) {
			f->GetLocation(&fx,&fy);
			d = Distance(x,y,fx,fy);
			if (d < lowest){
				lowest = d;
				r = (float)atan2((float)(fx-x),(float)(fy-y));
			}
			}
			f = GetNextObjective(&myit);
		}
	}

	// Problem - this breaks down if we're to near the front. Try another algorythm for close in stuff:
	if (lowest < 5){
		int		i;

		f = FindNearestObjective(x, y, NULL);
		if (!f->IsFrontline()){
			return r;		// This won't work unless we find a frontline objective. So just use the value from above
		}

		for (i=0,lowest=999.0F; i<f->static_data.links; i++){
			n = f->GetNeighbor(i);
			if (n && n->GetTeam() != f->GetTeam()){
				n->GetLocation(&fx, &fy);
				d = Distance(x, y, fx, fy);
				if (d < lowest){
					lowest = d;
					r = (float)atan2((float)(fx-x),(float)(fy-y));
				}
			}
		}
	}
	return r;
}

// Special case of above. Find direction toward friendly territory 
// (basically direction to front + PI if it's from one of our objectives)
float DirectionTowardFriendly (GridIndex x, GridIndex y, int team){
	float       d,r,lowest=999.0F;
	GridIndex   fx,fy;
	Objective   f,n;


	r = -1.0F;
	{
		VuListIterator	myit(FrontList);
		f = GetFirstObjective(&myit);
		while (f != NULL){
			// RV - Biker - This is a hack because JimG did introduce 
			if (f->GetType() != TYPE_BORDER || f->GetSType() != 2) {
			f->GetLocation(&fx,&fy);
			d = Distance(x,y,fx,fy);
			if (d < lowest){
				lowest = d;
				r = (float)atan2((float)(fx-x),(float)(fy-y));
				if (f->GetTeam() == team){
					r += (float)PI;
				}
			}
			}
			f = GetNextObjective(&myit);
		}
	}
	// Problem - this breaks down if we're to near the front. Try another algorythm for close in stuff:
	if (lowest < 5){
		int		i;

		f = FindNearestObjective(x,y,NULL);
		if ((f == NULL) || !f->IsFrontline()){
			// This won't work unless we find a frontline objective. So just use the value from above
			return r;		
		}
		for (i=0,lowest=999.0F; i<f->static_data.links; i++){
			n = f->GetNeighbor(i);
			if (n && n->GetTeam() != f->GetTeam()){
				n->GetLocation(&fx,&fy);
				d = Distance(x,y,fx,fy);
				if (d < lowest){
					lowest = d;
					r = (float)atan2((float)(fx-x),(float)(fy-y));
					if (n->GetTeam() != team)
						r += (float)PI;
				}
			}
		}
	}

	return r;
}

int GetBearingDeg (float x, float y, float tx, float ty){
	float		theta;
	theta = (float)atan ( (tx - x) / (ty - y) );
	return FloatToInt32(theta * RTD);
}

int GetRangeFt (float x, float y, float tx, float ty){
	return FloatToInt32(Distance(x,y,tx,ty));
}

void* PackXY (GridIndex x, GridIndex y){
	long     t;

	t = x | (y<<16);
	return (void*) t;
}

void UnpackXY(void* n, GridIndex* x, GridIndex* y){
	*x =(GridIndex) ((long)n & 0xFFFF);
	*y =(GridIndex) (((long)n>>16) & 0xFFFF);
}

void Trim(GridIndex* x, GridIndex* y){
	if (*x < 0)
		*x = 0;
	if (*x >= Map_Max_X)
		*x = (GridIndex)(Map_Max_X-1);
	if (*y < 0)
		*y = 0;
	if (*y >= Map_Max_Y)
		*y = (GridIndex)(Map_Max_Y-1);
}

float AngleTo(GridIndex ox, GridIndex oy, GridIndex tx, GridIndex ty){
	int		dx,dy;
	float	deg;

	dx = tx-ox;
	dy = ty-oy;
	if (!dx && !dy)
		return 0.0F;
	deg = (float)atan2((float)dx,(float)dy);
	if (deg < 0)
		deg += 2.0F*(float)PI;
	return deg;
}

CampaignHeading DirectionTo (GridIndex ox, GridIndex oy, GridIndex tx, GridIndex ty){
	int		dx,dy;
	float	deg;
	CampaignHeading	h;

	dx = tx-ox;
	dy = ty-oy;
	if (!dx && !dy){
		return Here;
	}
	deg = (float)atan2((float)dx,(float)dy);
	if (deg < 0){
		deg += 2.0F*(float)PI;
	}
	deg += .3839F;												// Shift by 22 degress;
	h = (CampaignHeading)(FloatToInt32((deg * 1.273F))%8);		// convert from 6.28 = 360 (2 PI) to 8=360;
	return h;
}

CampaignHeading DirectionTo(GridIndex ox, GridIndex oy, GridIndex tx, GridIndex ty, GridIndex cx, GridIndex cy){
	int				dx,dy;
	GridIndex		nx,ny;
	float			d,td;

	dx = tx-ox;
	dy = ty-oy;
	if (cx==tx && cy==ty)
		return Here;
	td = Distance(ox,oy,tx,ty);
	d = Distance(ox,oy,cx,cy);
	if (d < 1.9 || d > td)
		return DirectionTo(cx,cy,tx,ty);
	d += 1.9F;
	nx = (short)(ox+(GridIndex)(d/td*dx));
	ny = (short)(oy+(GridIndex)(d/td*dy));
	return DirectionTo(cx,cy,nx,ny);
}

int OctantTo (GridIndex ox, GridIndex oy, GridIndex tx, GridIndex ty)
{
	if (ox > tx){
		if (oy > ty){
			if (ox-tx > oy-ty)
				return 5;
			else
				return 4;
		}
		else {
			if (ox-tx > ty-oy)
				return 6;
			else
				return 7;
		}
	}
	else {
		if (oy > ty){
			if (tx-ox > oy-ty)
				return 2;
			else
				return 3;
		}
		else {
			if (tx-ox > ty-oy)
				return 1;
			else
				return 0;
		}
	}
}

int OctantTo (float ox, float oy, float tx, float ty)
{
	if (oy > ty){
		if (ox > tx){
			if (oy-ty > ox-tx)
				return 5;
			else
				return 4;
		}
		else {
			if (oy-ty > tx-ox)
				return 6;
			else
				return 7;
		}
	}
	else {
		if (ox > tx){
			if (ty-oy > ox-tx)
				return 2;
			else
				return 3;
		}
		else {
			if (ty-oy > tx-ox)
				return 1;
			else
				return 0;
		}
	}
}

float GridToSim(GridIndex x){
	return (float)(x*GRID_SIZE_FT)+OffsetToMiddle;
}

GridIndex SimToGrid(float x){
	// sfr: added fast float function
	return (GridIndex)FloatToInt32(x/GRID_SIZE_FT);
}

/* sfr: VERY IMPORTANT NOTE: 
* Sim and Grid coordinates are XY inverted. So this convertion is correct.
* Boths starts at lower left though.
* This explains why in sim coords Z- is up. (right hand from vertical to horizontal, thumbs down
*/
// Convert from Grid to sim coordinate systems
void ConvertGridToSim(GridIndex x, GridIndex y, vector *pos){
	pos->x = GridToSim(y);
	pos->y = GridToSim(x);
}

// Converts Sim to Grid coordinates
void ConvertSimToGrid(vector *pos, GridIndex *x, GridIndex *y){
	*x = SimToGrid(pos->y);
	*y = SimToGrid(pos->x);
}

MoveType AltToMoveType (int alt){
	if (alt >= LOW_ALTITUDE_CUTOFF){
		return Air;
	}
	if (alt > 0){
		return LowAir;
	}
	return Wheeled;				// Default movement type
}

int GetAltitudeLevel (int alt){
	int i;
	for (i=0; i<ALT_LEVELS; i++){
		if (alt < MaxAltAtLevel[i]){
			return i;
		}
	}
	ShiWarning("Altitude To High");
	return ALT_LEVELS-1;
}

int GetAltitudeFromLevel(int level, int seed){
	// This is a no-brainer.
	if (!level){
		return 0;
	}

	// Just in case this was set already (ie: a mission replan)
	// Kinda hackish - I should really fix this eventually
	if (level >= ALT_LEVELS){
		return level*GRIDZ_SCALE_FACTOR;
	}

	return MinAltAtLevel[level] + LevelIncrement[level]*(seed%IncrementMax[level]);
}

// speed should be in [distance units]/[hour]
CampaignTime TimeToArrive (float distance, float speed){
	if (!distance){
		return 0;
	}
	if (!speed){
		return 0xffffffff;
	}
	return  FloatToInt32((distance * CampaignHours) / speed);
}

// Speed should be in [grid units]/[hour]
CampaignTime TimeTo (GridIndex x, GridIndex y, GridIndex tx, GridIndex ty, int speed){
	return TimeToArrive(Distance(x,y,tx,ty),(float)speed);
}

CampaignTime TimeBetween (GridIndex x, GridIndex y, GridIndex tx, GridIndex ty, int speed){
	float d;

	d = Distance(x,y,tx,ty);
	return (CampaignTime) ((d/speed)*CampaignHours);
}

CampaignTime TimeBetweenO (Objective o1, Objective o2, int speed)
{
	GridIndex x,y,tx,ty;

	o1->GetLocation(&x,&y);
	o2->GetLocation(&tx,&ty);
	return TimeBetween (x,y,tx,ty,speed);
}									 

Objective FindObjective(VU_ID id){
	VuEntity*		e;
	e = vuDatabase->Find(id);
	if (e && GetEntityClass(e) == CLASS_OBJECTIVE){
		return (Objective)e;
	}
	return NULL;
}

Unit FindUnit(VU_ID id){
	VuEntity*		e;
	e = vuDatabase->Find(id);
	if (e && GetEntityClass(e) == CLASS_UNIT){
		return (Unit)e;
	}
	return NULL;
}

CampEntity FindEntity (VU_ID id){
	VuEntity*		e;
	e = vuDatabase->Find(id);
	if (e && (GetEntityClass(e) == CLASS_OBJECTIVE || GetEntityClass(e) == CLASS_UNIT)){
		return (CampEntity)e;
	}
	return NULL;
}

CampEntity GetEntityByCampID(int id){
	VuListIterator	myit(AllCampList);
	CampEntity		e;
	e = (CampEntity) myit.GetFirst();
	while (e){
		if (e->GetCampID() == id){
			return e;
		}
		e = (CampEntity) myit.GetNext();
	}
	return NULL;
}

// Finds nearest supply source
Objective FindNearestSupplySource(Objective o){
	Objective		c;
	int				n,who;
	float			cost;

	// Reset search array
	memset(CampSearch,0,sizeof(uchar)*MAX_CAMP_ENTITIES);

	FalconPrivateList	looklist(&AllObjFilter);
	looklist.ForcedInsert(o);
	who = o->GetTeam();

	// Keep branching out in objective links until I find a supply source
// #pragma message ("Somebody else look at this and see if I did it right")
	// Looks fine to me - RH
#if 0
	while (o = GetFirstObjective(&myit))
		{
		CampSearch[o->GetCampID()] = 1;
		for (n=0; n<o->static_data.links; n++)
			{
			c = o->GetNeighbor(n);
			if (c && !CampSearch[c->GetCampID()])
				{
				cost = GetObjectiveMovementCost(o,NULL,n,Wheeled,who,PATH_MARINE);
				if (cost < 250)
					{
					if (c->IsSupplySource())
						return c;
					looklist.ForcedInsert(c);
					}
				}
			}
		looklist.Remove(o);
		}
#endif	
	VuListIterator		myit(&looklist);
	o = GetFirstObjective(&myit);
	while (o){
		CampSearch[o->GetCampID()] = 1;
		for (n=0; n<o->static_data.links; n++){
			c = o->GetNeighbor(n);
			if (c && !CampSearch[c->GetCampID()]){
				cost = GetObjectiveMovementCost(o,NULL,n,Wheeled,(uchar)who,PATH_MARINE);
				if (cost < 250)
				{
					if (c->IsSupplySource())
						return c;
					looklist.ForcedInsert(c);
				}
			}
		}
		looklist.Remove(o);
		o = GetFirstObjective(&myit);
	}

	return NULL;
}

Unit FindNearestEnemyUnit(GridIndex X, GridIndex Y, GridIndex mx){
	GridIndex	max_dist = MAX_GROUND_SEARCH;
	int         d,nd,ld=0;
	Unit		u,n=NULL;
	GridIndex   x,y;
	if (max_dist){
		max_dist = mx;
	}

	::vector p;
	ConvertGridToSim(X, Y, &p);

#ifdef VU_GRID_TREE_Y_MAJOR
	VuGridIterator	myit(RealUnitProxList, p.y, p.x, (BIG_SCALAR)GridToSim(max_dist));
#else
	VuGridIterator	myit(RealUnitProxList, p.x, p.y, (BIG_SCALAR)GridToSim(max_dist));
#endif

	nd = max_dist*2;
	u = (Unit) myit.GetFirst();
	while (u != NULL){		
		u->GetLocation(&x, &y);
		d = FloatToInt32(Distance(X, Y, x, y));
		if (d>ld && d<nd){
			n = u;
			nd = d;
		}
		u = (Unit) myit.GetNext();
	}
	return n;
}

Unit FindNearestRealUnit(GridIndex X, GridIndex Y, float *last, GridIndex mx){
	GridIndex	max_dist = MAX_GROUND_SEARCH;
	float		lds,ds,nds=FLT_MAX;
	Unit		u,n=NULL;
	GridIndex   x,y;

	if (mx){
		max_dist = mx;
	}
	if (last == NULL || *last < 0){
		lds = -1.0F;
	}
	else {
		lds = (float)(*last * *last) + 0.1F;
	}

	::vector p;
	ConvertGridToSim(X, Y, &p);

#ifdef VU_GRID_TREE_Y_MAJOR
	VuGridIterator	myit(RealUnitProxList, p.y, p.x, (BIG_SCALAR)GridToSim(max_dist));
#else
	VuGridIterator	myit(RealUnitProxList, p.x, p.y, (BIG_SCALAR)GridToSim(max_dist));
#endif
	u = (Unit) myit.GetFirst();
	while (u != NULL){		
		u->GetLocation(&x, &y);
		ds = (float)DistSqu(X, Y, x, y);
		if (ds>lds && ds<nds){
			n = u;
			nds = ds;
		}
		u = (Unit) myit.GetNext();
	}
	if (last != NULL){
		*last = (float)sqrt(nds);
	}
	return n;
}

Unit FindNearestUnit(VuFilteredList* l, GridIndex X, GridIndex Y, float *last){	
	Unit        u,n=NULL;
	GridIndex   x,y;
	float		ds,nds=FLT_MAX,lds;

	if (last == NULL || *last < 0){
		lds = -1.0F;
	}
	else{
		lds = (float)(*last * *last) + 0.1F;
	}

	VuListIterator	myit(l);
	u = GetFirstUnit(&myit);
	while (u != NULL){
		u->GetLocation(&x, &y);
		ds = (float)DistSqu(X,Y,x,y);
		if (ds>lds && ds<nds && x>=0 && y>=0){
			n = u;
			nds = ds;
		}
		u = GetNextUnit(&myit);
	}

	if (last != NULL){
		*last = (float)sqrt(nds);
	}
	return n;
}

// This can be optimized for real units with the proximity list
Unit FindNearestUnit(GridIndex X, GridIndex Y, float *last){
	return FindNearestUnit(AllUnitList, X, Y, last);
}

Unit FindUnitByXY(VuFilteredList* l, GridIndex X, GridIndex Y, int domain){
	Unit        u;
	GridIndex   x,y;

	VuListIterator	myit(l);
	u = GetFirstUnit(&myit);
	while (u != NULL){
		u->GetLocation(&x, &y);
		if (x == X && y == Y && (u->GetDomain() == domain || domain < 1)){
			return u;
		}
		u = GetNextUnit(&myit);
	}
	return NULL;
}

// This can be optimized for real units with the proximity list
Unit GetUnitByXY(GridIndex X, GridIndex Y, int domain){
	return FindUnitByXY(AllUnitList, X, Y, domain);
}

// This can be optimized for real units with the proximity list
Unit GetUnitByXY(GridIndex X, GridIndex Y){
	Unit        u;
	GridIndex   x,y;

	VuListIterator	myit(AllUnitList);
	u = GetFirstUnit(&myit);
	while (u != NULL){
		u->GetLocation(&x, &y);
		if (x == X && y == Y){
			return u;
		}
		u = GetNextUnit(&myit);
	}
	return NULL;
}

Objective FindNearestObjective(VuFilteredList* l, GridIndex X, GridIndex Y, float *last){
	float		ds, nds=FLT_MAX, lds;
	Objective   o,n=NULL;
	GridIndex   x,y;

	if (last == NULL || *last < 0){
		lds = -1;
	}
	else{
		lds = (float)(*last * *last);
	}

	VuListIterator	myit(l);
	o = GetFirstObjective(&myit);
	while (o != NULL){		
		o->GetLocation(&x, &y);
		ds = (float) DistSqu(X, Y, x, y);
		if (ds>lds && ds<nds){
			n = o;
			nds = ds;
		}
		o = GetNextObjective(&myit);
	}

	if (last != NULL){
		*last = (float)sqrt(nds);
	}
	return n;
}

Objective FindNearestObjective(GridIndex X, GridIndex Y, float *last, GridIndex maxdist){
	::vector p;
	ConvertGridToSim(X, Y, &p);

#ifdef VU_GRID_TREE_Y_MAJOR
	VuGridIterator	myit(ObjProxList, p.y, p.x, (BIG_SCALAR)GridToSim(maxdist));
#else
	VuGridIterator	myit(ObjProxList, p.x, p.y, (BIG_SCALAR)GridToSim(maxdist));
#endif

	float		ds,lds,nds=FLT_MAX;
	Objective   o,n=NULL;
	GridIndex   x,y;

	if (last == NULL || *last < 0){
		lds = -1.0F;
	}
	else {
		lds = (float)(*last * *last) + 0.1F;
	}

	o = (Objective) myit.GetFirst();
	while (o != NULL){		
		o->GetLocation(&x, &y);
		ds = (float) DistSqu(X, Y, x, y);
		if (ds>lds && ds<nds){
			n = o;
			nds = ds;
		}
		o = (Objective) myit.GetNext();
	}
	if (last != NULL){
		*last = (float)sqrt(nds);
	}
	return n;
}

Objective FindNearestObjective(GridIndex X, GridIndex Y, float *last){
	return FindNearestObjective (X,Y,last,MAX_GROUND_SEARCH);
}

Objective FindNearestAirbase(GridIndex X, GridIndex Y){
	::vector p;
	ConvertGridToSim(X, Y, &p);

#ifdef VU_GRID_TREE_Y_MAJOR
	VuGridIterator	myit(ObjProxList, p.y, p.x, (BIG_SCALAR)GridToSim(100));
#else
	VuGridIterator	myit(ObjProxList, p.x, p.y, (BIG_SCALAR)GridToSim(100));
#endif

	int         d,nd;
	Objective   o,n=NULL;
	GridIndex   x,y;
	
	nd = 9999;
	o = (Objective) myit.GetFirst ();

	while (o != NULL){
		if (
			(o->GetType () == TYPE_AIRBASE) ||
			(o->GetType () == TYPE_AIRSTRIP)
		){
			o->GetLocation (&x, &y);

			d = FloatToInt32 (Distance (X, Y, x, y));
			if (d < nd){
				n = o;
				nd = d;
			}
		}
		o = (Objective) myit.GetNext ();
	}

	return n;
}

Objective FindNearbyAirbase(GridIndex X, GridIndex Y){
	::vector p;
	ConvertGridToSim(X, Y, &p);

	// sfr: from 5 to 9 because of seoul airbase
#ifdef VU_GRID_TREE_Y_MAJOR
	VuGridIterator	myit(ObjProxList, p.y, p.x, (BIG_SCALAR)GridToSim(5));
#else
	VuGridIterator	myit(ObjProxList, p.x, p.y, (BIG_SCALAR)GridToSim(5));
#endif

	int         d,nd;
	Objective   o,n=NULL;
	GridIndex   x,y;
	
	nd = 9999;
	o = (Objective) myit.GetFirst ();

	while (o != NULL){
		if (
			(o->GetType() == TYPE_AIRBASE) ||
			(o->GetType() == TYPE_AIRSTRIP)
		){
			o->GetLocation (&x, &y);
			d = FloatToInt32(Distance (X, Y, x, y));
			if (d < nd)	{
				n = o;
				nd = d;
			}
		}
		o = (Objective) myit.GetNext ();
	}
	return n;
}

Objective FindNearestFriendlyAirbase (Team who, GridIndex X, GridIndex Y){
	int         d,nd;
	Objective   o,n=NULL;
	GridIndex   x,y;
	
	nd =  9999; //100 km (in ft)

	VuListIterator	myit(AllObjList);
	o = (Objective) myit.GetFirst ();
	while (o != NULL){
		if ( 
			(GetTTRelations(o->GetTeam(),who) <= Neutral) && 
			o->GetType () == TYPE_AIRBASE && 
			o->brain && o->brain->NumOperableRunways() // JB 010729 CTD
		){
			o->GetLocation (&x, &y);
			d = FloatToInt32 (Distance (X, Y, x, y));
			if (d < nd){
				n = o;
				nd = d;
			}
		}
		o = (Objective) myit.GetNext ();
	}
	
	return n;
}

Objective FindNearestFriendlyRunway(Team who, GridIndex X, GridIndex Y){
	::vector p;
	ConvertGridToSim(X, Y, &p);

#ifdef VU_GRID_TREE_Y_MAJOR
	VuGridIterator	myit(ObjProxList, p.y, p.x, (BIG_SCALAR)GridToSim(100));
#else
	VuGridIterator	myit(ObjProxList, p.x, p.y, (BIG_SCALAR)GridToSim(100));
#endif

	int         d,nd;
	Objective   o,n=NULL;
	GridIndex   x,y;
	
	nd =  32800000; //100 km (in ft)
	o = (Objective) myit.GetFirst ();
	
	while (o != NULL){
		if (GetTTRelations(o->GetTeam(),who) <= Neutral){
			if(
				(o->GetType () == TYPE_AIRBASE) ||
				(o->GetType () == TYPE_AIRSTRIP)
			){
				o->GetLocation (&x, &y);
				d = FloatToInt32(Distance (X, Y, x, y));
				if (d < nd){
					n = o;
					nd = d;
				}
			}
		}
		o = (Objective) myit.GetNext ();
	}
	
	return n;
}

// This is a find objective routine optimized for friendly objectives
Objective FindNearestFriendlyObjective(Team who, GridIndex *x, GridIndex *y, int flags){
	Objective	o,bo=NULL;
	GridIndex	ox,oy,tx,ty;
	Int32		d,bd=999;

	::vector p;
	ConvertGridToSim(*x, *y, &p);

#ifdef VU_GRID_TREE_Y_MAJOR
	VuGridIterator	myit(ObjProxList, p.y, p.x, (BIG_SCALAR)GridToSim(MAX_GROUND_SEARCH));
#else
	VuGridIterator	myit(ObjProxList, p.x, p.y, (BIG_SCALAR)GridToSim(MAX_GROUND_SEARCH));
#endif

	ox = *x;
	oy = *y;
	o = (Objective) myit.GetFirst();
	while (o){
		if (GetTTRelations(o->GetTeam(),who) <= Neutral){
			if (flags & FF_SECONDLINE && o->IsFrontline()){
				o = (Objective) myit.GetNext();
				continue;
			}
			o->GetLocation(&tx,&ty);
			d = FloatToInt32(Distance(ox,oy,tx,ty));
			if (d < bd){
				bd = d;
				bo = o;
				*x = tx;
				*y = ty;
			}
		}
		o = (Objective) myit.GetNext();
	}
	return bo;
}

// This is a find objective routine optimized for friendly objectives
Objective FindNearestFriendlyObjective(VuFilteredList* l, Team who, GridIndex *x, GridIndex *y, int flags){
	Objective	o, bo=NULL;
	GridIndex	ox, oy, tx, ty;
	Int32		d, bd=999;
	ox = *x;
	oy = *y;

	VuListIterator	myit(l);
	o = GetFirstObjective(&myit);
	while (o){
		if (GetTTRelations(o->GetTeam(),who) <= Neutral){
			if (flags & FF_SECONDLINE && o->IsFrontline()){
				o = GetNextObjective(&myit);
				continue;
			}
			o->GetLocation(&tx,&ty);
			d = FloatToInt32(Distance(ox,oy,tx,ty));
			if (d < bd){
				bd = d;
				bo = o;
				*x = tx;
				*y = ty;
			}
		}
		o = GetNextObjective(&myit);
	}
	return bo;
}

// This is a find objective routine optimized for friendly power stations
Objective FindNearestFriendlyPowerStation(VuFilteredList* l, Team who, GridIndex x, GridIndex y){
    Objective	o,bo=NULL;
    GridIndex	tx,ty;
    Int32		d,bd=999;
    
	VuListIterator myit(l);
    for (o = GetFirstObjective(&myit); o; o = GetNextObjective(&myit)){
		if (GetTTRelations(o->GetTeam(),who) <= Neutral){
			if (
				o->GetType () == TYPE_NUCLEAR ||
				o->GetType () == TYPE_POWERPLANT
			){
				o->GetLocation(&tx,&ty);
				d = FloatToInt32(Distance(x,y,tx,ty));
				if (d < bd){
					bd = d;
					bo = o;
				}
			}
		}
    }
    return bo;
}

Objective GetObjectiveByXY(GridIndex X, GridIndex Y){
	::vector p;
	ConvertGridToSim(X, Y, &p);
#ifdef VU_GRID_TREE_Y_MAJOR
	VuGridIterator	myit(ObjProxList, p.y, p.x, (BIG_SCALAR)GridToSim(1));	
#else
	VuGridIterator	myit(ObjProxList, p.x, p.y, (BIG_SCALAR)GridToSim(1));
#endif
	Objective o;
	GridIndex x, y;

	o = (Objective) myit.GetFirst();
	while (o != NULL){
		o->GetLocation(&x, &y);
		if (x == X && y == Y){
			return o;
		}
		o = (Objective) myit.GetNext();
	}
	return NULL;
}

// Uses pre-calculated map data to do fast scoring of an x,y location
// Will return 0, 15, 30, 40, 45, 60, 70, 90 or 100 - Roughly % chance to hit.
int ScoreThreatFast(GridIndex X, GridIndex Y, int altlevel, Team who){
	int		i,ix,score = 0;
	Team	own;

	// Check vs territory ownership
	own = GetOwner(TheCampaign.CampMapData,X,Y);
	if (!GetRoE(who,own,ROE_AIR_OVERFLY)){
		return 32000;
	}
	if (!TheCampaign.SamMapData || !TheCampaign.RadarMapData){
		return 0;
	}

	// Find our indexes
	i = (Y/MAP_RATIO)*MRX + (X/MAP_RATIO);
	if (i < 0 || i > TheCampaign.SamMapSize){
		return 100;			// Off the map
	}
	if (who == FalconLocalSession->GetTeam()){
		ix = 4;
	}
	else {
		ix = 0;
	}
	
	// Now check vs threat at each altitude level
	switch (altlevel){
		case GroundAltitude:
			return 0;
		case LowAltitude:
			score = ((TheCampaign.SamMapData[i] >> ix) & 0x03) * 28;
			score += ((TheCampaign.SamMapData[i] >> (ix+2)) & 0x03) * 2;
			//score += ((TheCampaign.RadarMapData[i] >> ix) & 0x03) * 3;
			if (own && own != 0xF && GetRoE(who,own,ROE_AIR_FIRE))
				score += 10;		// 'General' threat for flying over enemy territory
			break;
		case MediumAltitude:
			score = ((TheCampaign.SamMapData[i] >> ix) & 0x03) * 10;
			score += ((TheCampaign.SamMapData[i] >> (ix+2)) & 0x03) * 23;
			//score += ((TheCampaign.RadarMapData[i] >> ix) & 0x03) * 1;
			//score += ((TheCampaign.RadarMapData[i] >> (ix+2)) & 0x03) * 2;
			break;
		case HighAltitude:
		default:
			score = ((TheCampaign.SamMapData[i] >> (ix+2)) & 0x03) * 30;
			//score += ((TheCampaign.RadarMapData[i] >> (ix+2)) & 0x03) * 3;
			break;
		case VeryHighAltitude:
			score = ((TheCampaign.SamMapData[i] >> (ix+2)) & 0x03) * 15;
			//score += ((TheCampaign.RadarMapData[i] >> (ix+2)) & 0x03) * 3;
			break;
	}
	return score;
}

int AnalyseThreats(GridIndex X, GridIndex Y, MoveType mt, int alt, int roe_check, Team who, int flags){
	Int32		d,da;
	int			threats=0;
	float		af;
	GridIndex   x,y;
	CampEntity	e;

	::vector p;
	ConvertGridToSim(X, Y, &p);

#ifdef VU_GRID_TREE_Y_MAJOR
	VuGridIterator	myit(RealUnitProxList, p.y, p.x, (BIG_SCALAR)GridToSim(MAX_AIR_SEARCH));
#else
	VuGridIterator	myit(RealUnitProxList, p.x, p.y, (BIG_SCALAR)GridToSim(MAX_AIR_SEARCH));
#endif

	af = alt * 0.000303F * 3;	// Convert feet to km * 3 (weight altitude heavy)
	af = af*af;						// square it
	e = (CampEntity) myit.GetFirst();
	while (e){
		e->GetLocation(&x,&y);
		d = da = FloatToInt32(Distance(X,Y,x,y));

		if (flags & FIND_CAUTIOUS){
			d = da = FloatToInt32(0.8F * d);
		}

		if (e->GetDetectionRange(mt) > d && GetRoE(e->GetTeam(),who,roe_check)){
			if (
				e->IsUnit() && !(flags & FIND_NOAIR && e->GetDomain() == DOMAIN_AIR) &&
				!(flags & FIND_NOMOVERS && ((Unit)e)->Moving())
			){
				d = d;		// placeholder. This unit is valid
			}
			else if (e->IsObjective() && ((Objective)e)->GetObjectiveStatus() > 30){
				d = d;		// placeholder. This objective is valid
			}
			else {
				// Continue looking...
				e = (CampEntity) myit.GetNext();
				continue;
			}
			
			if (af > 1.0F){
				da = FloatToInt32((float)sqrt((float)(d*d) + af));
			}

			if (e->GetAproxCombatStrength(mt,da)){
				threats += 4;
			}
			else if (d > VisualDetectionRange[mt] && !(flags & FIND_NODETECT)){
				threats++;
			}
		}
		e = (CampEntity) myit.GetNext();
	}
	return threats;
}

/*int CollectThreats(GridIndex X, GridIndex Y, int Z, Team who, int flags, F4PFList foundlist)
{
	Int32		d,hc,got,found=0,alt=0,pass=0;
	float		temp;
	MoveType	mt;
	GridIndex   x,y;
	CampEntity	e;
	uchar		tteam[NUM_TEAMS];
	VuGridIterator*	myit;

	::vector p;
	ConvertGridToSim(X, Y, &p);

	if (Z > 3300.0F){						// Over 1 km altitude
		temp = Z * 0.000303F;				// Convert feet to km
		alt = FloatToInt32(temp*temp)/2;	// Adjust alt, to be checked against range
	}
	mt = Air;
	if (Z < LOW_ALTITUDE_CUTOFF)
		mt = LowAir;
	
	// Set up roe checks
	for (d=0; d<NUM_TEAMS && TeamInfo[d]; d++){
		tteam[d] = (uchar)GetRoE((uchar)d,who,ROE_AIR_ENGAGE);
	}

	// Check lists
	while (pass < 2){
		if (!pass){
#ifdef VU_GRID_TREE_Y_MAJOR
			myit = new VuGridIterator(RealUnitProxList, p.y, p.x, (BIG_SCALAR)GridToSim(MAX_AIR_SEARCH));
#else
			myit = new VuGridIterator(RealUnitProxList, p.x, p.y, (BIG_SCALAR)GridToSim(MAX_AIR_SEARCH));
#endif
		}
		else {
#ifdef VU_GRID_TREE_Y_MAJOR
			myit = new VuGridIterator(ObjProxList, p.y, p.x, (BIG_SCALAR)GridToSim(MAX_AIR_SEARCH));
#else
			myit = new VuGridIterator(ObjProxList, p.x, p.y, (BIG_SCALAR)GridToSim(MAX_AIR_SEARCH));
#endif
		}
		e = (CampEntity) myit->GetFirst();
		while (e)
		{
			got = 0;
			if (tteam[e->GetTeam()])
			{
				if (e->IsUnit() && 
					!(flags & FIND_NOMOVERS && ((Unit)e)->Moving()) && 
					!(flags & FIND_NOAIR && e->GetDomain() == DOMAIN_AIR) &&
					(flags & FIND_FINDUNSPOTTED || e->GetSpotted(who)))
					d = d;		// placeholder. This unit is valid
				else if (e->IsObjective())
					d = d;		// placeholder. This objective is valid
				else
				{
					e = (CampEntity) myit->GetNext();
					continue;
				}
				e->GetLocation(&x,&y);
				d = FloatToInt32(Distance(X,Y,x,y));
				if (flags & FIND_CAUTIOUS)
					d = FloatToInt32(0.8F*d);
				if (!(flags & FIND_NODETECT) && e->GetDetectionRange(mt) > d)
					got++;
				hc = e->GetAproxHitChance(mt,d);
				if (hc > 0){
					if (alt > d)
						hc = e->GetAproxHitChance(mt,alt);
					if (hc > 0)
						got = 1;	
				}
				if (got && foundlist)
					foundlist->ForcedInsert(e);
				found += got;
			}
			e = (CampEntity) myit->GetNext();
		}
		delete myit;
		pass++;
	}
	return found;
}
*/

int CollectThreatsFast(GridIndex X, GridIndex Y, int altlevel, Team who, int flags, F4PFList foundlist){
	ShiAssert(FALSE == F4IsBadReadPtr(foundlist, sizeof *foundlist));
	if (F4IsBadReadPtr(foundlist, sizeof *foundlist)) // JB 010304 CTD
		return 0; // JB 010304 CTD

	Int32		d,retval=0,pass=0;
	MoveType	mt;
	GridIndex   x,y;
	CampEntity	e;
	int		tteam[NUM_TEAMS];
	VuListIterator* myit;

	if (altlevel < HighAltitude){
		mt = LowAir;
	}
	else {
		mt = Air;
	}

	// Set up roe checks
	for (d=0; d<NUM_TEAMS && TeamInfo[d]; d++){
		tteam[d] = GetRoE((uchar)d,who,ROE_AIR_ENGAGE);
	}

	// Check lists
	while (pass < 2){
		if (!pass){
			myit = new VuListIterator(AirDefenseList);
		}
		else{
			myit = new VuListIterator(EmitterList);
		}

		for (e = (CampEntity) myit->GetFirst(); e; e = (CampEntity) myit->GetNext()){
			if (e == NULL || e->GetTeam() < 0 || e->GetTeam() >= NUM_TEAMS){
				continue; // something bogus
			}
			if (tteam[e->GetTeam()] == ROE_NOT_ALLOWED){
				// not enemy
				continue;
			}

		    if (
				(!e->IsUnit() || !((Unit)e)->Moving()) && 
#if VU_ALL_FILTERED
				!foundlist->Find(e)
#else
				!foundlist->Find(e->Id())
#endif
			){
				e->GetLocation(&x,&y);
				d = FloatToInt32(Distance(X,Y,x,y));
				if (e->GetAproxHitChance(mt,d) > 0){
					foundlist->ForcedInsert(e);
					retval |= NEED_SEAD;
				}
				else if (!(flags & FIND_NODETECT) && e->GetDetectionRange(mt) > d){
					foundlist->ForcedInsert(e);
					retval |= NEED_ECM;
				}
		    }
		}
		delete myit;
		if (flags & FIND_NODETECT){
			pass = 10;		// Skip detector pass, essentially
		}
		pass++;
	}
	return retval;
}

void FillDistanceList (List list, Team who, int  i, int j){
	Objective   o;
	float       d;
	int         good;
	Team        own;
	GridIndex   x,y,lx,ly;
	void*			loc;
	ListNode		lp;

	list->Purge();

	VuListIterator	myit1(AllObjList);
	o = GetFirstObjective(&myit1);
	while (o != NULL){
		good = 0;
		o->GetLocation(&x,&y);
		own = o->GetTeam();
		if (GetTTRelations(who,own) == Allied || !own){
			if (o->IsFrontline())
				d = 0.0F;
			else
				d = DistanceToFront(x,y);
			if (d > (float)i && d < (float)j){
				good = 1;
				lp = list->GetFirstElement();
				while (lp && good){
					loc = lp->GetUserData();
					UnpackXY (loc, &lx, &ly);
					if (Distance(x,y,lx,ly) < i*1.5)
						good = 0;
					lp = lp->GetNext();
				}
			}
		}

		if (good){
			o->GetLocation(&x,&y);
			loc = PackXY(x,y);
			list->InsertNewElementAtEnd((short)FloatToInt32(DistanceToFront(x,y)), loc, 0);
		}
		o = GetNextObjective(&myit1);
	}
}

FalconSessionEntity* FindPlayer(Flight flight, uchar planeNum){
	if(flight){
		VuSessionsIterator	sessionWalker(FalconLocalGame);
		FalconSessionEntity *curSession;

		curSession = (FalconSessionEntity*)sessionWalker.GetFirst();
		while (curSession){
			if (curSession->GetPlayerFlightID() == flight->Id() && curSession->GetAircraftNum() == planeNum){
				return (curSession);
			}
			curSession = (FalconSessionEntity*)sessionWalker.GetNext();
		}
	}
	return(NULL);
}

FalconSessionEntity* FindPlayer(VU_ID flightID, uchar planeNum){
	Flight flight = (Flight) vuDatabase->Find(flightID);
	return FindPlayer(flight,planeNum);
}

FalconSessionEntity* FindPlayer(Flight flight, uchar planeNum, uchar pilotSlot){
	if(flight){
		VuSessionsIterator	sessionWalker(FalconLocalGame);
		FalconSessionEntity *curSession;

		curSession = (FalconSessionEntity*)sessionWalker.GetFirst();
		while (curSession){
			if (
				curSession->GetPlayerFlightID() == flight->Id() &&
				curSession->GetAircraftNum() == planeNum && 
				curSession->GetPilotSlot() == pilotSlot
			){
				return (curSession);
			}
			curSession = (FalconSessionEntity*)sessionWalker.GetNext();
		}
	}
	return(NULL);
}

FalconSessionEntity* FindPlayer(VU_ID flightID, uchar planeNum, uchar pilotSlot){
	Flight flight = (Flight) vuDatabase->Find(flightID);
	return FindPlayer(flight, planeNum, pilotSlot);
}


