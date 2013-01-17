
#ifndef NOFLY_H
#define NOFLY_H

#include "vutypes.h"
#include "Cmpglobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Team.h"

// Types of no fly zones:
#define NFZ_OWNER_NOFLY		0				// Only owner team and allies not allowed here (ie: SAM defense zone)
#define NFZ_OVERFLY_ONLY	1				// Only people with overfly privledges allowed (ie country borders)

// No-fly zone class
class NoFlyZoneClass;
typedef NoFlyZoneClass *NFZ;

class NoFlyZoneClass {
 	private:
	public:
  		GridIndex			zonex;			// Location of zone
		GridIndex			zoney;
		short					radius;
		uchar					owner;			// Who 'owns' this NFZ. Determines who can modify it
		uchar					type;				// what type of NoFlyZone is it?
		NFZ					next;
	public:
		NoFlyZoneClass(void);
		int InZone(GridIndex x, GridIndex y, Team who, int flags);
		void SetNFZ(Team who, int type);
	};

// ================================
// Global Functions
// ================================

extern NFZ AddNFZ (GridIndex x, GridIndex y, short radius);

extern void RemoveNFZ (NFZ zone, NFZ prev);

extern void RemoveNFZ (NFZ zone);

extern void DeleteZones (void);

extern int LoadNFZs (char *name);

extern int SaveNFZs (char *name);

extern int CheckZones (GridIndex x, GridIndex y, Team who);

extern int CheckZones (GridIndex x, GridIndex y, Team who, int flags);

extern int CheckZones (vector *location, Team who);

#endif