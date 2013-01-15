#ifndef _UI_Refresher_H_
#define _UI_Refresher_H_

enum
{
	GPS_NOTHING=0,
	GPS_FLIGHT,
	GPS_BATTALION,
	GPS_BRIGADE,
	GPS_DIVISION,
	GPS_TASKFORCE,
	GPS_OBJECTIVE,
};

class UI_Refresher
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_GENERAL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		short			Allowed_; // if a bit is set... allowed to update that section
		short			DivID_;
		short			CampID_;
		uchar			Type_;
		uchar			Side_;
		VU_ID			ID_;
		GlobalPositioningSystem *Owner_;

	public:
		C_Mission		*Mission_;
		MAPICONLIST		*MapItem_;
		C_ATO_Package	*Package_;
		C_ATO_Flight	*ATO_;
		C_Base			*OOB_;
		THREAT_LIST		*Threat_;

	public:
		UI_Refresher();
		~UI_Refresher();

		void Setup(CampEntity entity,GlobalPositioningSystem *owner,long allow);
		void Setup(Division div,GlobalPositioningSystem *owner,long allow);
		void Cleanup();

		void SetCampID(short campID) { CampID_=campID; }
		short GetCampID() { return(CampID_); }

		void SetID(VU_ID Id) { ID_=Id; }
		VU_ID GetID() { return(ID_); }

		void SetDivID(short id) { DivID_=id; }
		short GetDivID() { return(DivID_); }

		void SetType(uchar type) { Type_=type; }
		uchar GetType() { return(Type_); }

		void SetSide(uchar Side) { Side_=Side; }
		uchar GetSide() { return(Side_); }

		void Update(CampEntity entity,long allow);
		void Update(Division div,long allow);
		void Remove();

		void AddMission(CampEntity entity);
		void UpdateMission(CampEntity entity);
		void RemoveMission();

		void AddMapItem(CampEntity entity);
		void AddMapItem(Division div);
		void UpdateMapItem(CampEntity entity);
		void UpdateMapItem(Division div);
		void RemoveMapItem();

		void AddATOItem(CampEntity entity);
		void UpdateATOItem(CampEntity entity);
		void RemoveATOItem();

		void AddOOBItem(CampEntity entity);
		void AddOOBItem(Division div);
		void UpdateOOBItem(CampEntity entity);
		void UpdateOOBItem(Division div);
		void RemoveOOBItem();
};

#endif
