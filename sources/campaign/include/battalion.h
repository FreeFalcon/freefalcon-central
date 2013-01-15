
#ifndef BATTALION_H
#define BATTALION_H

#include "Gndunit.h"

//#define BAT_PLAN_AHEAD		16	
					
// =========================
// Battalion Class
// =========================

class BattalionClass : public GroundUnitClass {
#ifdef USE_SH_POOLS
public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(BattalionClass) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(BattalionClass), 200, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif

private:
	Percentage		supply;					// Unit statistics
	Percentage      fatigue;
	Percentage      morale;
	uchar			final_heading;			// Unit facing, at destination
	uchar			heading;				// Formation heading
	uchar			fullstrength;			// Number of vehicles at fullstrength
	uchar			radar_mode;				// Radar mode
	uchar			position;
	uchar			search_mode;			// Radar Search mode
	uchar			missiles_flying;		// Number of missiles being guided
	VU_TIME SEARCHtimer;
	VU_TIME AQUIREtimer;
	uchar			step_search_mode;		// 2002-03-04 ADDED BY S.G. The search mode used by radar stepping

	int				dirty_battalion;
	CampaignTime	last_resupply_time;		// Last time this unit received supplies

	public:
	CampaignTime	last_move;				// Time we moved last
	CampaignTime	last_combat;				// Last time this entity fired its weapons
	VU_ID			parent_id;				// Brigade parent, if present
	VU_ID			last_obj;				// The last objective this unit visited
	VU_ID			air_target;				// The ID of any air target (in addition to regular target)
	#ifdef USE_FLANKS
	GridIndex		lfx,lfy;				// Left flank
	GridIndex		rfx,rfy;				// Right flank
	#endif
	// sfr: changed to pointer, because of stream functions
	SmallPathClass	*path;					// The unit's path
	//		uchar           element;     			// Unit's position
	UnitDeaggregationData	*deag_data;		// Position data of previously deaggregated elements

public:
	// constructors and serial functions
	BattalionClass(ushort type, Unit parent);
	BattalionClass(VU_BYTE **stream, long *rem);
	virtual ~BattalionClass();
	virtual void InitData();
private:
	void InitLocalData(Unit parent);
public:
	virtual int SaveSize (void);
	virtual int Save (VU_BYTE **stream);

	// event Handlers
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

	virtual int IsBattalion (void)						{ return TRUE; }

	// Access Functions
	uchar GetFinalHeading (void) { return final_heading; }
	uchar GetFullStrength (void) { return fullstrength; }
	void SetFinalHeading (uchar);
	void SetFullStrength (uchar);

	// Required pure virtuals handled by BattalionClass
	virtual int MoveUnit (CampaignTime time);
	virtual int DoCombat (void);
	virtual UnitDeaggregationData* GetUnitDeaggregationData (void);
	virtual void ClearDeaggregationData (void);
	virtual int GetDeaggregationPoint (int slot, CampEntity *ent);
	virtual int Reaction (CampEntity what, int zone, float range);
	virtual int ChooseTactic (void);
	virtual int CheckTactic (int tid);
	virtual int Real (void)								{ return 1; }
	virtual void SetUnitOrders (int o, VU_ID oid);
	void PickFinalLocation (void);
	//		virtual void SetUnitAction (void);
	virtual int GetCruiseSpeed() const;
	virtual int GetCombatSpeed() const;
	virtual int GetMaxSpeed() const;
	virtual int GetUnitSpeed() const;
#if HOTSPOT_FIX
	virtual CampaignTime MaxUpdateTime() const;
#else
	virtual CampaignTime UpdateTime ();
#endif
	virtual CampaignTime CombatTime (void)				{ return GROUND_COMBAT_CHECK_INTERVAL*CampaignSeconds; }
	virtual int GetUnitSupplyNeed (int total);
	virtual int GetUnitFuelNeed (int total);
	virtual void SupplyUnit (int supply, int fuel);
	virtual int GetDetectionRange (int mt);				// Takes into account emitter status
	virtual int GetElectronicDetectionRange (int mt);	// Max Electronic detection range, even if turned off
	virtual int GetRadarMode (void)						{ return radar_mode; }
	virtual int GetSearchMode (void)						{ return search_mode; }
	// 2001-06-27 MODIFIED BY S.G. DIFFERENT DECLARATION THEN FROM FalcEnt.h RESULTS IN IT NOT BEING CALLED!
	// sfR: modified base class
	virtual void SetRadarMode (uchar mode)				{ radar_mode = mode; }
	//		virtual void SetRadarMode (int mode)				{ radar_mode = mode; }
	virtual void ReturnToSearch (void);
	// 2001-06-27 MODIFIED BY S.G. DIFFERENT DECLARATION THEN FROM FalcEnt.h RESULTS IN IT NOT BEING CALLED!
	// sfr: modified base class
	virtual void SetSearchMode (uchar mode)				{ search_mode = mode; }
	//		virtual void SetSearchMode (int mode)				{ step_search_mode = search_mode = mode; } // 2002-03-22 MODIFIED BY S.G. Init our step_search_mode as well
	virtual int CanShootWeapon (int wid);
	virtual int StepRadar (int t, int d, float range); //me123 modifyed to take tracking/detection range parameter
	virtual int GetVehicleDeagData (SimInitDataClass *simdata, int remote);
	virtual int GetMissilesFlying (void)				{ return missiles_flying; }//me123

	// core functions
	virtual void SetUnitLastMove (CampaignTime t)		{ last_move = t; }
	virtual void SetCombatTime (CampaignTime t)			{ last_combat = t; }
	virtual void SetUnitParent (Unit p)					{ parent_id = p->Id(); }
	virtual void SetUnitSupply (int s);
	virtual void SetUnitFatigue (int f);
	virtual void SetUnitMorale (int m);
	virtual void SetUnitHeading (uchar h)			 	{ heading = h; }
	virtual void SetUnitNextMove (void)					{ path->StepPath(); MakeBattalionDirty(DIRTY_SMALLPATH, SEND_SOON); }
	virtual void ClearUnitPath (void)					{ path->ClearPath(); MakeBattalionDirty(DIRTY_SMALLPATH, SEND_SOON); }
	virtual void SetLastResupplyTime (CampaignTime t)	{ last_resupply_time = t; }
	virtual void SetUnitPosition (uchar p)				{ position = p; }
	virtual void SimSetLocation (float x, float y, float z);
	virtual void GetRealPosition (float *x, float *y, float *z);
	virtual void HandleRequestReceipt(int type, int them, VU_ID flight);

	virtual CampaignTime GetMoveTime (void);
	virtual CampaignTime GetCombatTime (void)			{ return (TheCampaign.CurrentTime>last_combat)? TheCampaign.CurrentTime - last_combat:0; }
	virtual Unit GetUnitParent() const					{ return (Unit)vuDatabase->Find(parent_id); }
	virtual VU_ID GetUnitParentID (void)				{ return parent_id; }
	virtual VU_ID GetAirTargetID (void)					{ return air_target; }
	virtual FalconEntity* GetAirTarget (void)			{ return (FalconEntity*)vuDatabase->Find(air_target); }
	virtual void SetAirTarget (FalconEntity *t)			{ if (t) air_target = t->Id(); else air_target = FalconNullId; }
	void IncrementMissileCount (void)					{ missiles_flying++; SetRadarMode(FEC_RADAR_GUIDE); }
	void DecrementMissileCount (void)					{ missiles_flying--; if (missiles_flying == 0) ReturnToSearch(); ShiAssert( missiles_flying>=0); }
	#ifdef USE_FLANKS
	virtual void GetLeftFlank (GridIndex *x, GridIndex *y)	{ *x = lfx; *y = lfy; }
	virtual void GetRightFlank (GridIndex *x, GridIndex *y)	{ *x = rfx; *y = rfy; }
	#endif
	virtual int GetUnitSupply (void)					{ return (int)supply; }
	virtual int GetUnitFatigue (void)					{ return (int)fatigue; }
	virtual int GetUnitMorale (void)					{ return (int)morale; }
	virtual int GetUnitHeading (void)					{ return (int)heading; }
	virtual int GetNextMoveDirection (void)				{ return path->GetNextDirection(); MakeBattalionDirty(DIRTY_SMALLPATH, SEND_SOON); }
	virtual int GetUnitElement (void);
	virtual int GetUnitPosition (void)					{ return position; }
	virtual int RallyUnit (int minutes);
	virtual CampaignTime GetLastResupplyTime (void)		{ return last_resupply_time; }

	// Support functions
	float GetSpeedModifier() const;
	virtual float AdjustForSupply(void);
	virtual void IncrementTime (CampaignTime dt) 		{ last_move += dt; }

	// Dirty Data
	void MakeBattalionDirty (Dirty_Battalion bits, Dirtyness score);
	void WriteDirty (unsigned char **stream);
	void ReadDirty (unsigned char **stream, long *rem);
	// 2002-03-22 ADDED BY S.G. Needs them outside of battalion class
	virtual void SetAQUIREtimer(VU_TIME newTime)		{ AQUIREtimer = newTime; };
	virtual void SetSEARCHtimer(VU_TIME newTime)		{ SEARCHtimer = newTime; };
	virtual void SetStepSearchMode(uchar mode)			{ step_search_mode = mode; };
	virtual VU_TIME GetAQUIREtimer(void)				{ return AQUIREtimer; };
	virtual VU_TIME GetSEARCHtimer(void)				{ return SEARCHtimer; };
	// END OF ADDED SECTION 2002-03-22
};

typedef BattalionClass*	Battalion;

BattalionClass* NewBattalion (int type, Unit parent);

#endif
