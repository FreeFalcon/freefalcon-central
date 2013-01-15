//
// This includes Division, Brigade and Battalion classes
//

#ifndef GNDUNIT_H
#define GNDUNIT_H

#include "unit.h"
#include "AIInput.h"
#include "SIM/INCLUDE/gndai.h"

class AircraftClass;

//	==========================================
// Orders and roles available to ground units
// ==========================================

#define GORD_RESERVE			0
#define GORD_CAPTURE			1
#define GORD_SECURE				2			// Secure all objectives around the assigned objective
#define GORD_ASSAULT			3			// Amphibious assault
#define GORD_AIRBORNE			4			// Airborne assault
#define GORD_COMMANDO			5			// Commando raid - Land behind lines and cause damage
#define GORD_DEFEND				6
#define GORD_SUPPORT			7
#define GORD_REPAIR				8
#define GORD_AIRDEFENSE			9
#define GORD_RECON				10
#define GORD_RADAR				11			// Generally radar units just detecting stuff
#define GORD_LAST				12

extern int OrderPriority[GORD_LAST];		// Update this if new orders are added

#define GRO_RESERVE				0
#define GRO_ATTACK				1
#define GRO_ASSAULT				2
#define GRO_AIRBORNE			3
#define GRO_DEFENSE				4
#define GRO_AIRDEFENSE			5
#define GRO_FIRESUPPORT			6
#define GRO_ENGINEER			7
#define GRO_RECON				8
#define GRO_LAST				9

// =========================
// Ground Formations
// =========================

//sfr: this is defined only once, at Gndai.h
/*#define GFORM_DISPERSED			0			// Scattered / Disorganized
#define GFORM_COLUMN			1			// Your standard column
#define GFORM_OVERWATCH			3			// Cautious column
#define GFORM_WEDGE				4
#define GFORM_ECHELON			5
#define GFORM_LINE				6*/

#define FF_SECONDLINE			0x01
#define FF_LOSTOK				0x02

// ============================================
// Ground unit positions
// ============================================

#define GPOS_NONE		0
#define GPOS_RECON1		1
#define GPOS_RECON2		2
#define GPOS_RECON3		3
#define GPOS_COMBAT1	4
#define GPOS_COMBAT2	5
#define GPOS_COMBAT3	6
#define GPOS_RESERVE1	7
#define GPOS_RESERVE2	8
#define GPOS_RESERVE3	9
#define GPOS_SUPPORT1	10
#define GPOS_SUPPORT2	11
#define GPOS_SUPPORT3	12

#define WPA_FINAL		0
#define WPA_ENROUTE		1

#define MAX_SUPPORT_DIST	20
#define MAX_NORMAL_DIST		20

// =========================
// Ground Unit Class
// =========================

class GroundUnitClass : public UnitClass {
private:
	uchar				orders;    		// Current orders
	short				division;		// What division it belongs to (abstract)
	VU_ID				pobj;			// Primary objective we're assigned to
	VU_ID				sobj;			// Secondary objective we're attached to
	VU_ID				aobj;			// Actual objective we're assigned to do something with

	int					dirty_ground_unit;

public:
	// constructors and serial functions
	GroundUnitClass(ushort type, VU_ID_NUMBER id);
	GroundUnitClass(VU_BYTE **stream, long *rem);
	virtual ~GroundUnitClass();
	virtual int SaveSize (void);
	virtual int Save (VU_BYTE **stream);

	// event Handlers
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

	// Required pure virtuals handled by GroundUnitClass
	virtual MoveType GetMovementType (void);
	virtual MoveType GetObjMovementType (Objective o, int n);
	virtual int DetectOnMove (void);
	virtual int ChooseTarget (void);
#if HOTSPOT_FIX
	virtual CampaignTime MaxUpdateTime() const		{ return GROUND_UPDATE_CHECK_INTERVAL*CampaignSeconds; }
#else
	virtual CampaignTime UpdateTime (void)			{ return GROUND_UPDATE_CHECK_INTERVAL*CampaignSeconds; }
#endif
    virtual int OnGround (void)						{ return TRUE; }
    virtual float GetVt() const						{ return (Moving() ? 40.0F : 0.0F);}
    virtual float GetKias() const					{ return GetVt()*FTPSEC_TO_KNOTS; }

	// Access functions
	uchar GetOrders (void) { return orders; }
	short GetDivision (void) { return division; }

	void SetOrders (uchar);
	void SetDivision (short);
	void SetPObj (VU_ID);
	void SetSObj (VU_ID);
	void SetAObj (VU_ID);

	// Core functions
	int DetectVs (AircraftClass *ac, float *d, int *combat, int *spotted, int *capture, int *nomove, int *estr);
	int DetectVs (CampEntity e, float *d, int *combat, int *spotted, int *capture, int *nomove, int *estr);
	virtual void SetUnitOrders (uchar o)			{ orders = o; }
	virtual void SetUnitDivision (short d)			{ division = d; }
	void SetUnitPrimaryObj (VU_ID id)				{ pobj = id; }
	void SetUnitSecondaryObj (VU_ID id)				{ sobj = id; }
	void SetUnitObjective (VU_ID id)				{ aobj = id; }
	virtual int GetUnitOrders (void)				{ return (int)orders; }
	virtual int GetUnitDivision (void)				{ return (int)division; }
	Objective GetUnitPrimaryObj (void)				{ return (Objective)vuDatabase->Find(pobj); }
	Objective GetUnitSecondaryObj (void)			{ return (Objective)vuDatabase->Find(sobj); }
	Objective GetUnitObjective (void)				{ return (Objective)vuDatabase->Find(aobj); }
	VU_ID GetUnitPrimaryObjID (void)				{ return pobj; }
	VU_ID GetUnitSecondaryObjID (void)				{ return sobj; }
	VU_ID GetUnitObjectiveID (void)					{ return aobj; }

	virtual int CheckForSurrender (void);
	virtual int GetUnitNormalRole (void);
	virtual int GetUnitCurrentRole() const;
	virtual int BuildMission(void);

	void MakeGndUnitDirty (Dirty_Ground_Unit bits, Dirtyness score);
	void WriteDirty (unsigned char **stream);
	//sfr: changed here
	void ReadDirty (unsigned char **stream, long *rem);
};

#include "Battalion.h"
#include "Brigade.h"

// ============================
// Global functions
// ============================

extern char* DirectionToEnemy (char* buf, GridIndex x, GridIndex y, Team who);

extern void ReorderRallied (Unit u);

extern Unit BestElement(Unit u, int at, int role);

extern int FindNextBest (int d, int pos[]);

extern void ReorganizeUnit (Unit u);

extern int BuildGroundWP (Unit u);

extern void GetCombatPos (Unit e, int positions[], char ed[], GridIndex* tx, GridIndex *ty);

extern int GetActionFromOrders (int orders);

extern int CheckUnitStatus (Unit u);

extern int CheckReady (Unit u);

extern int CalculateOpposingStrength(Unit u, F4PFList list);

extern int SOSecured(Objective o, Team who);

extern int GetActionFromOrders(int orders);

extern CampaignHeading GetAlternateHeading (Unit u, GridIndex x, GridIndex y, GridIndex nx, GridIndex ny, CampaignHeading h);

extern int ScorePosition (GridIndex x, GridIndex y, GridIndex px, GridIndex py, int position, int ours);

extern Objective FindBestPosition(Unit u, Unit e, F4PFList nearlist);

extern void ClassifyUnitElements (Unit u, int *recon, int *combat, int *reserve, int *support);

extern int GetPositionOrders (Unit e);

extern void FindBestCover (GridIndex x, GridIndex y, CampaignHeading h, GridIndex *cx, GridIndex *cy, int roadok);

extern Objective FindRetreatPath(Unit u, int depth, int flags);

extern Unit RequestArtillerySupport (Unit req, Unit target);

extern int RequestCAS (int Team, Unit target);

extern int RequestSupport (Unit req, Unit target);

extern void RequestOCCAS (Unit u, GridIndex x, GridIndex y, CampaignTime time);

extern void RequestBAI (Unit u, GridIndex x, GridIndex y, CampaignTime time);

extern int GetGroundRole (int orders);

extern int GetGroundOrders (int role);

#endif
