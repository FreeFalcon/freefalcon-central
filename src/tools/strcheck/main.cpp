#include <stdio.h>
#include <stdlib.h>
#include "Entity.h"
#include "Vehicle.h"
#include "Vu2.h"

int ShowTable (uchar domain, FILE* fp);

extern short NumVehicleEntries;
extern short NumWeaponTypes;

// =========================
// VU required globals
// =========================

static unsigned long gameStartTime;
ulong vuxVersion = 1;
SM_SCALAR vuxTicsPerSec = 1000.0F;
VuSlaveSmootherSettings *vuxSlaveSettings = 0;
VU_TIME vuxCurrentTime = 0;
VU_TIME vuxTransmitTime = 0;
VuSessionEntity *vuxLocalSessionEntity = 0;
ulong vuxLocalDomain = 1;	// range = 1-31
VU_BYTE vuxLocalSession = 1;
char *vuxWorldName = "EBS";
VU_TIME vuxRealTime = 0;

// =================================
// VU related globals for Falcon 4.0
// =================================

VuMainThread *gMainThread = 0;
Falcon4EntityClassType* Falcon4ClassTable;
F4CSECTIONHANDLE* vuCritical = NULL;
int NumEntities;
VU_ID FalconNullId;

// ======================================
// Main function
// ======================================

int main(void)
	{
	FILE *fp;

	if (!LoadClassTable("D:/Falcon4/Campaign/Save/Falcon4"))
		{
		printf("No entities loaded.\n");
		exit(0);
		}
	fp = fopen("output.txt","w");
	ShowTable(DOMAIN_LAND,fp);
	ShowTable(DOMAIN_SEA,fp);
	ShowTable(DOMAIN_AIR,fp);
	fclose(fp);
	return 0;
	}

char DamStr[11][4] = { "NOD", "Pen", "HEx", "Hea", "Inc", "Prx", "Kin", "Hyd", "Chm", "Nuc", "Oth" };
uchar DamTypes[11] = { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 };

int BestVehicleWeapon (int id, uchar* dam, MoveType m, int range)
	{
	int			i,j,str,bs,w,wl,ws,bw=-1,bhp=-1;
	VehicleClassDataType*	vc;

	vc = (VehicleClassDataType*) Falcon4ClassTable[id].dataPtr;
	if (!vc)
		return 0;
	bw = bs = 0;
	for (i=0; i<HARDPOINT_MAX; i++)
		{
		w = vc->Weapon[i];
		ws = vc->Weapons[i];
		if (ws == 255)
			{
			wl = w;
			for (j=0; j<MAX_WEAPONS_IN_LIST; j++)
				{
				w = GetListEntryWeapon(wl,j);
				if (w < NumWeaponTypes)
					{
					str = GetWeaponScore(w, dam, m, range);
					if (str > bs)
						{
						bw = w;
						bs = str;
						bhp = i;
						}
					}
				}
			}
		if (w && ws)
			{
			str = GetWeaponScore (w, dam, m, range);
			if (str > bs)
				{
				bw = w;
				bs = str;
				bhp = i;
				}
			}
		}
	return bw;
	}

int ShowTable (uchar domain, FILE* fp)
	{
	int		i,j,c,str,wid;

	for (i=0,c=0; i<NumVehicleEntries; i++)
		{
		if (!(c%60))
			{
			fprintf(fp,"Name           HP  NoMove  Foot    Wheeled Tracked LowAir  Air     Naval   Rail    \n");
			c++;
			}
		if (Falcon4ClassTable[VehicleDataTable[i].Index].vuClassData.classInfo[VU_DOMAIN] == domain)
			{
			fprintf(fp,"%-14.14s %3d ",VehicleDataTable[i].Name,VehicleDataTable[i].HitPoints);
			for (j=0; j<=Rail; j++)
				{
				wid = BestVehicleWeapon(VehicleDataTable[i].Index, DamTypes, (MoveType)j, 0);
				str = GetWeaponScore(wid,j,0);
				fprintf(fp,"%3d/%s ",str,DamStr[WeaponDataTable[wid].DamageType]);
				}
			fprintf(fp,"\n");
			c++;
			}
		}
	return 0;
	}

/*

		short		Index;						// descriptionIndex pointing here
		short		Flags;
		char		Name[16];
		short		HitPoints;					// Damage this thing can take
		short		Data1;						// Generic Data slots: For A/C- Cruise and Max Alt.
		short		Data2;							
		uchar		CallsignIndex;
		uchar		CallsignSlots;
		uchar		HitChance[MOVEMENT_TYPES];	// Vehicle hit chances (best hitchance & bonus)
		uchar		Strength[MOVEMENT_TYPES];	// Combat strengths (full strength only) (calculated)
		uchar		Range[MOVEMENT_TYPES];		// Firing ranges (full strength only) (calculated)
		uchar		Detection[MOVEMENT_TYPES];	// Electronic detection ranges
		short		Weapon[HARDPOINT_MAX];		// Weapon id of weapons (or weapon list)
		uchar		Weapons[HARDPOINT_MAX];		// Number of shots each (fully supplied)
		uchar		DamageMod[OtherDam+1];		// How much each type will hurt me (% of strength applied)
		};
*/

// =============================
// Stubs
// =============================

int vuxComHandle;

char * ComAPIRecvBufferGet(int a)
	{
	return NULL;
	}

int	ComAPIGet(int a)
	{
	return 0;
	}

int ComAPIHostIDLen(int a)
	{
	return 0;
	}

int ComAPIHostIDGet(int a, char* b)
	{
	return 0;
	}

void ComAPIGroupSet(int a, VU_ID b)
	{
	}

VuMessage* VuxCreateMessage(unsigned short)
	{
	return NULL;
	}

VuEntity* VuxCreateEntity(ushort type, ushort size, VU_BYTE *data)
	{
	return NULL;
	}

VuEntityType *VuxType(ushort id)
	{
	return NULL;
	}
