#ifndef _RAIL_INFO_H_
#define _RAIL_INFO_H_

#include "sim/include/hardpnt.h" // MLR 2/25/2004 - 

class RailInfo // MLR 2/25/2004 - changed to class
{
public:
	RailInfo()  // MLR 2/25/2004 - 
	{ 
		//pylonID		= 0; 
		//rackID		= 0; 
		//weaponID	= 0;
		startBits	= 0;
		currentBits	= 0;
		weaponCount = 0;
	};

	AdvancedWeaponStation hardPoint; // MLR 2/25/2004 - 
	//short pylonID;
	//short rackID;
	//short weaponID;
	short startBits;
	short currentBits;
	short weaponCount;
};

struct RailList
{
	RailInfo rail[HARDPOINT_MAX];
};

#endif