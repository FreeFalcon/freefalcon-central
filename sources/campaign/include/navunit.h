
#ifndef NAVUNIT_H
#define NAVUNIT_H

#include "unit.h"
#include "AIInput.h"

// =========================
// Naval Unit defines
// =========================

#define NORD_NONE			0
#define NORD_ATTACK			1
#define NORD_BOMBARD		2
#define NORD_TRANSPORT		3
#define NORD_STATION		4
#define NORD_OTHER			5

#define NRO_NONE			0
#define NRO_ATTACK			1
#define NRO_BOMBARD			2
#define NRO_TRANSPORT		3
#define NRO_OTHER			4

#define NAV_PLAN_AHEAD		8

// =========================
// TaskForceClass
// =========================

class TaskForceClass : public UnitClass 
{
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size) { ShiAssert( size == sizeof(TaskForceClass) ); return MemAllocFS(pool);	};
    void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
    static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(TaskForceClass), 200, 0 ); };
    static void ReleaseStorage()	{ MemPoolFree( pool ); };
    static MEM_POOL	pool;
#endif

private:
	CampaignTime		last_move;
	CampaignTime		last_combat;
	uchar				orders;     		// Unit's mission
	uchar				last_direction;		// Direction of last move
	Percentage			supply;				// Unit's supply
	SmallPathClass		path;
	VU_ID				air_target;			// The ID of any air target (in addition to regular target)
	uchar missiles_flying;
	uchar			radar_mode;				// Radar mode
	uchar			search_mode;			// Radar Search mode
	VU_TIME SEARCHtimer ;
	VU_TIME AQUIREtimer ;
	uchar			step_search_mode;		// 2002-03-04 ADDED BY S.G. The search mode used by radar stepping
public:

	uchar					tacan_channel;				// Support for carriers
	uchar					tacan_band;					// Support for carriers

	
	TaskForceClass(ushort type);
	TaskForceClass(VU_BYTE **stream, long *rem);
	virtual ~TaskForceClass();
	virtual int SaveSize (void);
	virtual int Save (VU_BYTE **stream);

	// event Handlers
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

	// Required pure virtuals
	//MI added function for movement
	virtual float GetVt() const { return GetUnitSpeed() * KPH_TO_FPS; }
	virtual float GetKias() const { return GetVt() * FTPSEC_TO_KNOTS; }	
	virtual int DetectOnMove();
	virtual MoveType GetMovementType (void)			{ return Naval; }
	virtual int Reaction (CampEntity e, int zone, float range);
	virtual int MoveUnit (CampaignTime time);
	virtual int DoCombat (void);
	virtual int ChooseTactic (void);
	virtual int CheckTactic (int tid);
	virtual int Real (void)							{ return 1; }
	virtual int GetUnitSpeed() const;
	virtual VU_ID GetAirTargetID (void)				{ return air_target; }
	virtual FalconEntity* GetAirTarget (void)		{ return (FalconEntity*)vuDatabase->Find(air_target); }
	virtual void SetAirTarget (FalconEntity *t)		{ if (t) air_target = t->Id(); else air_target = FalconNullId; }
#if HOTSPOT_FIX
	virtual CampaignTime MaxUpdateTime() const		{ return NAVAL_MOVE_CHECK_INTERVAL*CampaignSeconds; }
#else
	virtual CampaignTime UpdateTime (void)			{ return NAVAL_MOVE_CHECK_INTERVAL*CampaignSeconds; }
#endif
	virtual CampaignTime CombatTime (void)			{ return NAVAL_COMBAT_CHECK_INTERVAL*CampaignSeconds; }
	virtual int IsTaskForce (void)					{ return TRUE; }
	virtual int OnGround (void)						{ return TRUE; }
	virtual int GetVehicleDeagData (SimInitDataClass *simdata, int remote);
	virtual int GetDeaggregationPoint (int slot, CampEntity *ent);

	// core functions
	virtual void SetUnitOrders (uchar o)				{ orders = o; }
	virtual void SetUnitSupply (uchar s)				{ supply = s; }
	virtual void SetUnitNextMove (void)				{ path.StepPath(); }
	virtual void ClearUnitPath (void)				{ path.ClearPath(); }
	virtual int GetNextMoveDirection (void)			{ return path.GetNextDirection(); }
	virtual int GetUnitOrders (void)				{ return orders; }
	virtual int GetUnitSupply (void)				{ return (int)supply; }
	virtual CampaignTime GetMoveTime (void);
	virtual void SetCombatTime (CampaignTime t)			{ last_combat = t; }
	virtual CampaignTime GetCombatTime (void)		{ return TheCampaign.CurrentTime - last_combat; }
	virtual void GetRealPosition (float *x, float *y, float *z);
	virtual int CanShootWeapon (int wid);
	virtual int GetRadarMode (void)						{ return radar_mode; }
// 2002-03-22 MODIFIED BY S.G. DIFFERENT DECLARATION THEN FROM FalcEnt.h RESULTS IN IT NOT BEING CALLED!
	// sfr: modified base class 
	virtual void SetRadarMode (uchar mode)				{ radar_mode = mode; }
//		virtual void SetRadarMode (int mode)				{ radar_mode = mode; }
	virtual void ReturnToSearch (void);
// 2002-03-22 MODIFIED BY S.G. DIFFERENT DECLARATION THEN FROM FalcEnt.h RESULTS IN IT NOT BEING CALLED!
	// sfr: modified base class
	virtual void SetSearchMode (uchar mode)				{ search_mode = mode; }
//		virtual void SetSearchMode (int mode)				{ step_search_mode = search_mode = mode; } // 2002-03-22 MODIFIED BY S.G. Init our step_search_mode as well
	virtual int StepRadar (int t, int d, float range);//me123 modifyed to take tracking/detection range parameter
	virtual int ChooseTarget (void);
	virtual void IncrementTime (CampaignTime dt) 		{ last_move += dt; }
	virtual void SetUnitLastMove (CampaignTime t)		{ last_move = t; }

	int DetectVs (AircraftClass *ac, float *d, int *combat, int *spot);
	int DetectVs (CampEntity e, float *d, int *combat, int *spot);
	void IncrementMissileCount (void)					{ missiles_flying++; SetRadarMode(FEC_RADAR_GUIDE); }
	void DecrementMissileCount (void)					{ missiles_flying--; if (missiles_flying == 0) ReturnToSearch(); ShiAssert( missiles_flying>=0); }
	virtual int GetMissilesFlying (void)				{ return missiles_flying; } // MLR 10/3/2004 - finishing what //me123 //Cobra 10/31/04 TJL

	// 2002-03-22 ADDED BY S.G. Needs them outside of nav unit class
	virtual void SetAQUIREtimer(VU_TIME newTime)		{ AQUIREtimer = newTime; };
	virtual void SetSEARCHtimer(VU_TIME newTime)		{ SEARCHtimer = newTime; };
	virtual void SetStepSearchMode(uchar mode)			{ step_search_mode = mode; };
	virtual VU_TIME GetAQUIREtimer(void)				{ return AQUIREtimer; };
	virtual VU_TIME GetSEARCHtimer(void)				{ return SEARCHtimer; };
};

typedef TaskForceClass *TaskForce;

TaskForceClass* NewTaskForce (int type);

#endif
