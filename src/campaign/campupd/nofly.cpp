#include <windows.h>
#include <stdio.h>
#include "ClassTbl.h"
#include "F4Find.h"
#include "NoFly.h"
#include "find.h"
#include "Campaign.h"


// Global No fly zone list
NFZ NFZList = NULL;

// ============================ 
// External Function Prototypes
// ============================

// ============================ 
// Class functions
// ============================

NoFlyZoneClass::NoFlyZoneClass(void)
	{
	zonex = zoney = 0;
	radius = 0;
	owner = 0;
	type = NFZ_OVERFLY_ONLY;
	next = NULL;
	}

int NoFlyZoneClass::InZone(GridIndex x, GridIndex y, Team who, int flags)
	{
	float			d;

	d = Distance(x,y,zonex,zoney);
	if (flags & FIND_CAUTIOUS)
		d *= 0.8F;
	if (d < radius)
		{
		if (type == NFZ_OVERFLY_ONLY)
			{
			if (!GetRoE(who,owner,ROE_AIR_OVERFLY))
				return 1;
			}
		else if (type == NFZ_OWNER_NOFLY)
			{
			if (GetTTRelations(who,owner) == Allied)
				return 1;
			}
		}
	return 0;
	}

void NoFlyZoneClass::SetNFZ(Team who, int t)
	{
	owner = who;
	type = t;
	}

// ================================
// Global Functions
// ================================

NFZ AddNFZ (GridIndex x, GridIndex y, short radius)
	{
	NFZ		zone;

	zone = new NoFlyZoneClass();
	zone->zonex = x;
	zone->zoney = y;
	zone->radius = radius;
	if (NFZList)
		zone->next = NFZList;
	NFZList = zone;
	return zone;
	}

void RemoveNFZ (NFZ zone, NFZ prev)
	{
	if (prev)
		prev->next = zone->next;
	else
		NFZList = zone->next;
	delete zone;
	}

void RemoveNFZ (NFZ zone)
	{
	NFZ		curr;

	curr = NFZList;
	if (curr == zone)
		NFZList = zone->next;
	else
		{
		while (curr && curr->next != zone)
			curr = curr->next;
		if (curr)
			curr->next = zone->next;
		}
	delete zone;
	}

void DeleteZones (void)
	{
	NFZ		zone,next;

	zone = NFZList;
	while (zone)
		{
		next = zone->next;
		delete zone;
		zone = next;
		}
	}

int LoadNFZs (char *name)
	{
	FILE*		fp;
	short		entries;
	NFZ			zone,last=NULL;

	if ((fp = OpenCampFile (name, "nfz", "rb")) == NULL)
		return 0;
	fread(&entries,sizeof(short),1,fp);
	while (entries)
		{
		zone = new NoFlyZoneClass();
		fread(zone,sizeof(NoFlyZoneClass),1,fp);
		if (!NFZList)
			NFZList = zone;
		if (last)
			last->next = zone;
		last = zone;
		}
	CloseCampFile(fp);
	return 1;
	}

int SaveNFZs (char *name)
	{
	FILE*		fp;
	short		zones = 0;
	NFZ			zone;

	if ((fp = OpenCampFile (name, "nfz", "wb")) == NULL)
		return 0;

	// Count # of zones
	zone = NFZList;
	while (zone)
		{
		zones++;
		zone = zone->next;
		}
	// Save them
	fwrite(&zones,sizeof(short),1,fp);
	zone = NFZList;
	while (zone)
		{
		fwrite(zone,sizeof(NoFlyZoneClass),1,fp);
		zone = zone->next;
		}
	CloseCampFile(fp);
	return 1;
	}

// Checks all zones to see if x,y is in one
int CheckZones (GridIndex x, GridIndex y, Team who)
	{
	NFZ	zone;

	zone = NFZList;
	while (zone)
		{
		if (zone->InZone(x,y,who,0))
			return 1;
		zone = zone->next;
		}
	return 0;
	}

int CheckZones (GridIndex x, GridIndex y, Team who, int flags)
	{
	NFZ	zone;

	zone = NFZList;
	while (zone)
		{
		if (zone->InZone(x,y,who,flags))
			return 1;
		zone = zone->next;
		}
	return 0;
	}

// Checks all zones in sim coordinates
int CheckZones (vector *location, Team who)
	{
	GridIndex x,y;

	ConvertSimToGrid((vector*)(&location),&x,&y);
	return CheckZones(x,y,who);
	}
