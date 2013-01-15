#ifndef _UI_STORES_LIST_H_
#define _UI_STORES_LIST_H_
//
// This class is basically a LIST Manager
// and I just wanted to keep it "structured"
//

#define _STORES_NAME_LEN_ (20)

typedef struct StoresStr STORESLIST;

struct StoresStr
{
	long ID;
	_TCHAR Name[_STORES_NAME_LEN_];
	long Type;
	long Weight;
	long Fuel;
	short Stock;
	float DragFactor;
	short HardPoint[HARDPOINT_MAX]; // 0 = Not Allowed
	STORESLIST *Next;
};

class StoresList
{
	public:

		enum
		{
			_AIR_TO_AIR_=0,
			_AIR_TO_GROUND_,
			_OTHER_,
			_ALL_, // # of lists
		};

		enum
		{
			_TYPE_MISSILE_=100,
			_TYPE_ROCKET_,
			_TYPE_BOMB_,
			_TYPE_FUEL_,
			_TYPE_OTHER_,
			_TYPE_GUN_,
		};

		STORESLIST *Stores_[_ALL_];
		long ListID_;
		long GetType_;
		STORESLIST *current;

	public:
		StoresList()
		{
			Stores_[_AIR_TO_AIR_]=NULL;
			Stores_[_AIR_TO_GROUND_]=NULL;
			Stores_[_OTHER_]=NULL;
			current=NULL;
			ListID_=_ALL_;
			GetType_=_ALL_;
		}
		~StoresList() {}

		void Cleanup();
		STORESLIST *Create(long ID,_TCHAR *Name,long Type,long wgt,long fuel,float df,short stock);
		void AddHardPoint(long ID,long hp,short count);
		STORESLIST *Find(long ID)
		{
			STORESLIST *cur;

			if(current)
			{
				if(current->ID == ID)
					return(current);
			}
			cur=GetFirst(_ALL_);
			while(cur)
			{
				if(cur->ID == ID)
					return(cur);
				cur=GetNext();
			}
			return(NULL);
		}
		void SetHardPoint(long ID,long hp,short value)
		{
			STORESLIST *cur;

			if(hp < HARDPOINT_MAX)
			{
				cur=Find(ID);
				if(cur)
					cur->HardPoint[hp]=value;
			}
		}
		void Add(STORESLIST *store,STORESLIST **list);
		void Add(STORESLIST *store,long ListID) { if(ListID < _ALL_) Add(store,&Stores_[ListID]); }
		void RemoveAll(STORESLIST **list);
		void RemoveAll(long ID) { if(ID < _ALL_) RemoveAll(&Stores_[ID]); }
		void Remove(long ID,STORESLIST **top);
		void Remove(long ID,long ListID) { if(ListID < _ALL_) Remove(ID,&Stores_[ListID]); }
		void Sort(long ID);
		void Sort()
		{
			long i;

			for(i=_AIR_TO_AIR_;i < _ALL_;i++)
				Sort(i);
		}
		STORESLIST *GetFirst(long ID)
		{
			GetType_=ID;
			if(GetType_ == _ALL_)
				ListID_=_AIR_TO_AIR_;
			else
				ListID_=ID;
			if(GetType_ == _ALL_)
			{
				current=NULL;
				while(!current && ListID_ < _ALL_)
				{
					current=Stores_[ListID_];
					if(!current)
						ListID_++;
				}
			}
			else
				current=Stores_[ListID_];

			return(current);
		}
		STORESLIST *GetNext()
		{
			if(current)
			{
				current=current->Next;
				if(current)
					return(current);
			}
			if(GetType_ == _ALL_ && ListID_ < _OTHER_)
			{
				ListID_++;
				current=Stores_[ListID_];	
				return(current);
			}
			return(NULL);
		}
		STORESLIST *GetAll() { return(GetFirst(_ALL_)); }
		STORESLIST *GetAirToAir() { return(GetFirst(_AIR_TO_AIR_)); }
		STORESLIST *GetAirToGround() { return(GetFirst(_AIR_TO_GROUND_)); }
		STORESLIST *GetOther() { return(GetFirst(_OTHER_)); }
};

#endif