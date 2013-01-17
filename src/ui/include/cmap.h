#ifndef _MAP_HANDLER_H_
#define _MAP_HANDLER_H_

#ifndef _UI_FILTERS_H_
#include "filters.h"
#endif

#ifndef _VC_HEADER_
#include "tac_class.h"
#endif

enum // Draw Flags
{
	_MAP_TEAM_0				= 0x00000001, // Calculation= (1 << i) (where i>=0 && i < _MAX_TEAMS_)
	_MAP_TEAM_1				= 0x00000002,
	_MAP_TEAM_2				= 0x00000004,
	_MAP_TEAM_3				= 0x00000008,
	_MAP_TEAM_4				= 0x00000010,
	_MAP_TEAM_5				= 0x00000020,
	_MAP_TEAM_6				= 0x00000040,
	_MAP_TEAM_7				= 0x00000080,
	_MAP_OBJECTIVES_		= 0x01000000, // Objectives on/off bit
	_MAP_UNITS_				= 0x02000000, // Units on/off bit
	_MAP_AIR_UNITS_			= 0x04000000, // Air Units on/off bit
	_MAP_NAVAL_UNITS_		= 0x08000000, // Naval Units on/off
	_MAP_WAYPOINTS_			= 0x10000000,
	_MAP_THREATS_			= 0x20000000,
	_MAP_BULLSEYE_			= 0x40000000, // Show Bullseye
};

typedef struct
{
	THREAT_CIRCLE *SamLow;
	THREAT_CIRCLE *SamHigh;
	THREAT_CIRCLE *RadarLow;
	THREAT_CIRCLE *RadarHigh;
} THREAT_LIST;

typedef struct
{ // This class is NEVER actually added to a window... used to make an overlay
	long Flags[_MAP_NUM_THREAT_TYPES_];
	C_Threat *Type[_MAP_NUM_THREAT_TYPES_];
} THR_LIST;

typedef struct
{
	long Flags[_MAP_NUM_OBJ_TYPES_];
	C_MapIcon *Type[_MAP_NUM_OBJ_TYPES_];
} OBJ_LIST;

typedef struct
{
	long Flags[_MAP_NUM_AIR_TYPES_];
	C_MapIcon *Type[_MAP_NUM_AIR_TYPES_];
} AIR_LIST;

typedef struct
{
	long Flags[_MAP_NUM_GND_LEVELS_];
	C_MapIcon *Levels[_MAP_NUM_GND_LEVELS_];
} GND_SIZE;

typedef struct
{
	long Flags[_MAP_NUM_GND_TYPES_];
	GND_SIZE *Type[_MAP_NUM_GND_TYPES_];
} GND_LIST;

typedef struct
{
	long Flags[_MAP_NUM_NAV_TYPES_];
	C_MapIcon *Type[_MAP_NUM_NAV_TYPES_];
} NAV_LIST;

typedef struct
{
	THR_LIST  *Threats; // These are used to create an overlay
	AIR_LIST  *AirUnits;
	GND_LIST  *Units;
	NAV_LIST  *NavalUnits;
	OBJ_LIST  *Objectives;
	C_Waypoint *Waypoints;
} MAPICONS;

class ObjectiveClass; typedef ObjectiveClass *Objective;
class UnitClass; typedef UnitClass *Unit;
class FlightClass; typedef FlightClass *Flight;

class C_Map
{
	private:
		long AirIconIDs_[_MAX_TEAMS_][_MAX_DIRECTIONS_][2]; // [Team 0-7][Heading][0 = Not Selected,1 = Selected]
		long ArmyIconIDs_[_MAX_TEAMS_][2]; // [Team 0-7][0 = Not Selected,1 = Selected]
		long NavyIconIDs_[_MAX_TEAMS_][2]; // [Team 0-7][0 = Not Selected,1 = Selected]
		long ObjIconIDs_[_MAX_TEAMS_][2]; // [Team 0-7][0 = Not Selected,1 = Selected]
		float CenterX_,CenterY_;
		long ZoomLevel_;
		long MinZoomLevel_;
		long MaxZoomLevel_;
		long ZoomStep_;
		float scale_; // calculated
		float maxy; // calculated... used for converting to my coordinates
		long flags_;
		short Circles_;

		float BullsEyeX_,BullsEyeY_;

		float LogMinX_,LogMinY_,LogMaxX_,LogMaxY_; // Min/Max ranges for WaypointZs
		float StrtMinX_,StrtMinY_,StrtMaxX_,StrtMaxY_; // Min/Max ranges for WaypointZs

		long ObjectiveMask_; // masks for displaying map icons
		long UnitMask_;
		long NavalUnitMask_;
		long AirUnitMask_;
		long ThreatMask_;

		long MapID;
		C_ScaleBitmap *Map_; // 1536x2048 16 bit map
		UI95_RECT MapRect_;

		long TeamFlags_[_MAX_TEAMS_];
		MAPICONS Team_[_MAX_TEAMS_];
		COLORREF TeamColor_[_MAX_TEAMS_];

		VU_ID WPUnitID_;  // Flight ID of current waypoints
		C_Waypoint *CurWP_; // Currently selected WP list
		C_Waypoint *CurWPZ_; // Currently selected WP list (altitudes only)
		RECT CurWPArea_; // Needs to be RECT not UI95_RECT

		C_DrawList *CurIcons_; // current icons for targets & airbases (will always be on when CurWP_ is displayed)

		C_Cursor *SmallMapCtrl_; // Keeps small map up to date :)
		C_Window *DrawWindow_,*WPZWindow_;
		UI95_RECT DrawRect_;

		C_BullsEye *BullsEye_;

		VU_ID CurFlight_;

		void CalculateDrawingParams();
		void BuildWPList(C_Waypoint *wplist,C_Waypoint *wpzlist,Unit unit);
		void BuildCurrentWPList(Unit unit);

		void AddListsToWindow();
		void RemoveListsFromWindow();
		void SetTeamScales();
		void ScaleMap();

	public:

		C_Map();
		~C_Map();

		void SetLogRanges(float minx,float miny,float maxx,float maxy) { LogMinX_=minx; LogMinY_=miny; LogMaxX_=maxx; LogMaxY_=maxy; } // Min/Max ranges for WaypointZs
		void SetStrtRanges(float minx,float miny,float maxx,float maxy) { StrtMinX_=minx; StrtMinY_=miny; StrtMaxX_=maxx; StrtMaxY_=maxy; } // Min/Max ranges for WaypointZs
		void SetAirIcons(long TeamNo,long Dir,long OffID,long OnID) { AirIconIDs_[TeamNo&7][Dir&7][0]=OffID; AirIconIDs_[TeamNo&7][Dir&7][1]=OnID; }
		void SetArmyIcons(long TeamNo,long OffID,long OnID) { ArmyIconIDs_[TeamNo&7][0]=OffID; ArmyIconIDs_[TeamNo&7][1]=OnID; }
		void SetNavyIcons(long TeamNo,long OffID,long OnID) { NavyIconIDs_[TeamNo&7][0]=OffID; NavyIconIDs_[TeamNo&7][1]=OnID; }
		void SetObjectiveIcons(long TeamNo,long OffID,long OnID) { ObjIconIDs_[TeamNo&7][0]=OffID; ObjIconIDs_[TeamNo&7][1]=OnID; }

		MAPICONS GetTeam(int i) { return Team_[i]; } // 2002-02-23 ADDED BY S.G. Need to exteriorize Team_
		void SetupOverlay();
		void Cleanup();
		void SetMapImage(long ID);
		C_Base *GetMapControl() { return(Map_); }
		void SetWindow(C_Window *win);
		C_Window *GetWindow() { return(DrawWindow_); }
		C_Window *GetZWindow() { return(WPZWindow_); }
		C_Waypoint *GetCurWP() { return(CurWP_); }
		C_Waypoint *GetCurWPZ() { return(CurWPZ_); }
		VU_ID GetCurWPID() { return(WPUnitID_); }
		void SetWPZWindow(C_Window *win);
		void SetTeamFlags(long TeamID,long flags) { if(TeamID >= 0 && TeamID < _MAX_TEAMS_) TeamFlags_[TeamID]=flags; }
		long GetTeamFlags(long TeamID) {  if(TeamID >= 0 && TeamID < _MAX_TEAMS_) return(TeamFlags_[TeamID]); return(0); }
		void SetTeamColor(long TeamID,COLORREF color) { if(TeamID >= 0 && TeamID < _MAX_TEAMS_) TeamColor_[TeamID]=color; }
		COLORREF GetTeamColor(long TeamID) {  if(TeamID >= 0 && TeamID < _MAX_TEAMS_) return(TeamColor_[TeamID]); return(0); }
		void FitFlightPlan();
		void SetZoomLevel(short zoom);
		void ZoomIn();
		void ZoomOut();
		void SetFlags(long flag) { flags_ |= flag; }
		void SetFlight(VU_ID ID) { CurFlight_=ID; }
		void SetBullsEye(float x,float y);
		long GetZoomLevel() { return(ZoomLevel_); }
		void SetUnitLevel(long level);
		void ShowObjectiveType(long mask);
		void HideObjectiveType(long mask);
		void ShowUnitType(long mask);
		void HideUnitType(long mask);
		void ShowAirUnitType(long mask);
		void HideAirUnitType(long mask);
		void RefreshAllAirUnitType(void); // 2002-02-21 ADDED BY S.G.
		void ShowNavalUnitType(long mask);
		void HideNavalUnitType(long mask);
		void ShowThreatType(long mask);
		void HideThreatType(long mask);
		void SetMapCenter(long x,long y);
		void MoveCenter(long x,long y);
		long GetMapCenterX() { return(FloatToInt32(CenterX_)); }
		long GetMapCenterY() { return(FloatToInt32(CenterY_)); }
		void SetSmallMap(C_Cursor *smap) { SmallMapCtrl_=smap; }
		float GetMaxY() { return(maxy); }
		float GetMapScale() { return(scale_); }
		BOOL SetWaypointList(VU_ID UnitID);
		BOOL SetCurrentWaypointList(VU_ID UnitID);
		void RemoveCurWPList();
		void RemoveAllWaypoints (short team);
		void RemoveWaypoints (short team,long group);
		void RemoveAllEntities();
		void RemoveFromCurIcons(long ID);
		void AddToCurIcons(MAPICONLIST *MapItem);
		void CenterOnIcon(MAPICONLIST *MapItem);
		THREAT_LIST *AddThreat(CampEntity ent);
		MAPICONLIST *AddObjective(Objective Obj);
		MAPICONLIST *AddDivision(Division div);
		MAPICONLIST *AddUnit(Unit u);
		MAPICONLIST *AddFlight(Flight flight);
		MAPICONLIST *AddSquadron(Squadron squadron);
		MAPICONLIST *AddPackage(Package package);
		MAPICONLIST *AddVC(victory_condition *vc);
		void UpdateWaypoint(Flight flt);
		void UpdateVC(victory_condition *vc);
		void RemoveVC(long team,long ID);
		void RemoveOldWaypoints();
		void TurnOnNames();
		void TurnOnBoundaries();
		void TurnOnArrows();
		void TurnOffNames();
		void TurnOffBoundaries();
		void TurnOffArrows();
		void TurnOnBullseye();
		void TurnOffBullseye();
		void DrawMap();
		void SetAllObjCallbacks(void (*cb)(long,short,C_Base*));
		void SetAllAirUnitCallbacks(void (*cb)(long,short,C_Base*));
		void SetAllGroundUnitCallbacks(void (*cb)(long,short,C_Base*));
		void SetAllNavalUnitCallbacks(void (*cb)(long,short,C_Base*));
		void SetObjCallbacks(long type,void (*cb)(long,short,C_Base*));
		void SetAirUnitCallbacks(long type,void (*cb)(long,short,C_Base*));
		void SetGroundUnitCallbacks(long level,long type,void (*cb)(long,short,C_Base*));
		void SetNavalUnitCallbacks(long type,void (*cb)(long,short,C_Base*));
		void SetUnitCallbacks(long level,long type,void (*cb)(long,short,C_Base*));
		C_MapIcon *GetObjIconList(long team,long type);
		void RecalcWaypointZs(long scaletype); // 1=Log, 2=straight
		void GetMapRelativeXY(short *x,short *y) 
		{	
			if(DrawWindow_) 
			{
				*x = static_cast<short>(*x - DrawWindow_->GetX() - DrawWindow_->VX_[0]); 
				*y = static_cast<short>(*y - DrawWindow_->GetY() - DrawWindow_->VY_[0]); 
			} 
		}

		void RemapTeamColors(long team);
};

#endif
