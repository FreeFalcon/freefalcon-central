#ifndef _GPS_H_
#define _GPS_H_

enum
{
	_GPS_MISSION_RESIZE_	=0x00000001,
	_GPS_MAP_REFRESH_		=0x00000002,
	_GPS_ATO_RESIZE_		=0x00000004,
	_GPS_OOB_RESIZE_		=0x00000008,
	_GPS_RESORT_MISSION_	=0x00000010,
	_GPS_HASH_SIZE_			=512,
};

enum
{
	UR_NOTHING	=0x00000000,
	UR_MISSION	=0x00000001,
	UR_MAP		=0x00000002,
	UR_ATO		=0x00000004,
	UR_SQUADRON	=0x00000008,
	UR_OOB		=0x00000020,

	UR_ALL		=0xffffffff, // Enable ALL

	UR_DIVISION =0x00010000, // Special Case THING for Divisions (NOT related to flags above)
};

class GlobalPositioningSystem
{
	private:
		long Allowed_;
		long Flags;
		C_Hash *GPS_Hash;
		C_HASHNODE *Cur_;
		long CurIdx_;

	public:
		C_Map *Map_;
		C_TreeList *MisTree_;
		C_TreeList *AtoTree_;
		C_TreeList *OOBTree_;

		long ObjectiveMenu_;
		long UnitMenu_;
		long WaypointMenu_;
		long SquadronMenu_;
		long MissionMenu_;

		long TeamNo_;

		GlobalPositioningSystem();
		~GlobalPositioningSystem();

		void Setup();
		void Cleanup();
		void Clear();

		void *Find(long CampID);

		void SetMap(C_Map *map)					{ Map_=map; }
		void SetMissionTree(C_TreeList *tree);
		void SetATOTree(C_TreeList *tree);
		void SetOOBTree(C_TreeList *tree);

		void SetObjectiveMenu(long ID)			{ ObjectiveMenu_=ID; }
		void SetUnitMenu(long ID)				{ UnitMenu_=ID; }
		void SetWaypointMenu(long ID)			{ WaypointMenu_=ID; }
		void SetSquadronMenu(long ID)			{ SquadronMenu_=ID; }
		void SetMissionMenu(long ID)			{ MissionMenu_=ID; }

		void SetFlags(long flag)				{ Flags = flag; }
		long GetFlags()							{ return(Flags); }
		void SetAllowed(long sections)			{ Allowed_=sections; }
		long GetAllowed()						{ return(Allowed_); }

		void SetTeamNo(long ID)					{ TeamNo_=ID; }
		long GetTeamNo()						{ return(TeamNo_); }

		void UpdateDivisions();
		void Update();

		void *GetFirst(); // These can onlybe used by one thread at a time
		void *GetNext();
};

#endif