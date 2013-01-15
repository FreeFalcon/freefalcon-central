#ifndef _LOADOUT_H_
#define _LOADOUT_H_

typedef struct
{
	short WeaponID[HARDPOINT_MAX];
	short WeaponCount[HARDPOINT_MAX];
} HPLIST;

typedef struct LoadoutStr LOADOUT;

struct LoadoutStr
{
	long ID; // CampID
	HPLIST Stores[5];
	LOADOUT *Next;
};

class C_Loadout
{
	public:
		LOADOUT *Root_;

	public:
		C_Loadout() { Root_=NULL; }
		~C_Loadout() {}

		void Setup() { Root_=NULL; }
		void Cleanup() { if(Root_) RemoveAll(); }
		LOADOUT *Create(long ID,HPLIST *storeslist);
		void Add(LOADOUT *load);
		LOADOUT *Find(long ID);
		void Remove(long ID);
		void RemoveAll();
};

#endif
