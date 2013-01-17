// 
// This includes the Flight and Squadron classes. See Package.cpp for Package class
//

#ifndef AIRUNIT_H
#define AIRUNIT_H


#include "AIInput.h"
#include "unit.h"

// =========================
// Types and Defines
// =========================

#define RESERVE_MINUTES				15					// How much extra fuel to load. Setable?

class AircraftClass;

//	==========================================
// Orders and roles available to air units
// ==========================================

#define ROLE_AWACS		22
#define ROLE_JSTAR		23
#define ROLE_TANKER		24
#define ROLE_ELINT		25

// =========================
// Air unit Class 
// =========================

class AirUnitClass : public UnitClass
{
public:
		// constructors and serial functions
		AirUnitClass(ushort type, VU_ID_NUMBER id);
		AirUnitClass(VU_BYTE **stream, long *rem);
		virtual ~AirUnitClass();
		virtual int SaveSize (void);
		virtual int Save (VU_BYTE **stream);

		// event Handlers
		virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

		// Required pure virtuals handled by AirUnitClass
		virtual MoveType GetMovementType (void);
		virtual int GetUnitSpeed() const;
#if HOTSPOT_FIX
		virtual CampaignTime MaxUpdateTime() const			{ return AIR_UPDATE_CHECK_INTERVAL*CampaignSeconds; }
#else
		virtual CampaignTime UpdateTime (void)				{ return AIR_UPDATE_CHECK_INTERVAL*CampaignSeconds; }
#endif
		// pure virtual implementation
		virtual float GetVt() const							{ return GetUnitSpeed() * KPH_TO_FPS; }
		virtual float GetKias()	const 						{ return GetVt() * FTPSEC_TO_KNOTS; }

		// core functions
		virtual int IsHelicopter (void) const;
		virtual int OnGround (void);
};

#include "Flight.h"
#include "Squadron.h"
#include "Package.h"


// =========================================
// Air Unit functions
// =========================================

int GetUnitScore (Unit u, MoveType mt);

#endif
