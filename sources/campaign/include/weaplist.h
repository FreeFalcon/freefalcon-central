
#ifndef WEAPLIST_H
#define WEAPLIST_H

#define MAX_WEAPONS_IN_LIST	64	// to accomodate additional weapons

struct WeaponListDataType {
	char		Name[16];
	short		WeaponID[MAX_WEAPONS_IN_LIST];
	uchar		Quantity[MAX_WEAPONS_IN_LIST];
	};

extern WeaponListDataType*		WeaponListDataTable;

// Functions

extern int GetListEntryWeapon(int list, int num);

extern int GetListEntryWeapons(int list, int num);

extern char* GetListName(int list);

#endif
