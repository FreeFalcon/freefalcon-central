#ifndef _INIT_DATA_H
#define _INIT_DATA_H

class FalconEntity;

#include "Campaign/Include/Campweap.h"

// ==============================
// Creation Flags
// ==============================

#define SIDC_SILENT_INSERT		0x01			// Don't broadcast a create event upon insertion
#define SIDC_REMOTE_OWNER		0x02			// We want to create this with a remote machine as owner
#define SIDC_FORCE_ID			0x04			// Assign an Id to this entity
#define SIDC_NO_INSERT			0x08			// Don't insert the entity

// =================================
// Forward Declarations for pointers
// =================================
class WayPointClass;
class ObjectiveClass;
class FalconSessionEntity;
class FalconEntity;
class CampBaseClass;

// =================================
// The Class
// =================================

class SimInitDataClass
{
public:
	enum CreateType {
		DebugVehicle = 1, UIObject,
		InstantActionEnemy, InstantActionPlayerPlane, 
		DogfightAIPlane, DogfightPlayerPlane,
		CampaignVehicle, CampaignFeature 
	};
	CreateType createType;                   ///< What type/where this entity is being created from
	Int32 createFlags;                       ///< Any special creation tasks
	FalconSessionEntity	*owner;              ///< Pointer to the session we want to own this
	// sfr: this is stupid, since both are of the same type and only one will be used.
	// either we make different types or use only one.
	// Im leaving only one
	//CampBaseClass* campUnit;               ///< Pointer to Campaign Entity (one of these will be NULL)
	//CampBaseClass* campObj;
	CampBaseClass *campBase;
	VU_ID         forcedId;                  ///< Id to force this entity to..
	Int32         missionType;               ///< which type (unit or obj it is)
	Int32         descriptionIndex;          ///< Description Index of the unit being created
	Int32         campSlot;                  ///< feature/vehicle ID within the campaign unit
	Int32         inSlot;                    ///< the xth vehicle in the above campSlot
	Int16         playerSlot;                ///< Which pilot controls this vehicle (0-3)
	Int16         vehicleInUnit;             ///< xth vehicle in a unit - ie: If unit had 15 vehicles, this would be 0-14.
	Int16         skill;                     ///< 0-4. Pilot/Crew experience level
	Int32         status;                    ///< State of the entity to be created
	Int32         side;                      ///< Owning country
	Float32       x, y, z;                   ///< Position Data
	Float32       heading;                   ///< Heading, in radians
	Int32         fuel;                      ///< Fuel, in lbs - A/C Only
	Int32         flags;                     ///< campaign flags
	Int32         specialFlags;              ///< flags in special data
	Int32         ptIndex;                   ///< Index into our pt list, if any.
	Int32         rwIndex;                   ///< Runway index (which runway is being used for T/O or landing, if any.
	Int32		  start_stalled;								 ///< Start in a deep stall;
	short weapon[HARDPOINT_MAX];             ///< Weapon ID of each hardpoint	- A/C Only
	unsigned char weapons[HARDPOINT_MAX];    ///< Weapons on that harpoint		- A/C Only
	Int32         numWaypoints;              ///< Number of waypoints
	Int32         currentWaypoint;           ///< Waypoint the unit is currently heading towards
	Int32         callsignIdx;               ///< Index into the Callsign array (ie: Viper/Cowboy/etc)
	Int32         callsignNum;               ///< Flight num of callsign (ie: Viper1/Viper2/Viper3/etc)
	WayPointClass *waypointList;             ///< List of waypoints
	uchar		  displayPriority;							 ///< Detail level at which to display this entity
};

#endif
