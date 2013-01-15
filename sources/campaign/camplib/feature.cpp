// ***************
// feature.cpp
// ***************

#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include "cmpglobl.h"
#include "feature.h"

#include "classtbl.h"  // JB 010106 CTD sanity check

FeatureClassDataType* GetFeatureClassData (int index)
	{
	ShiAssert ( Falcon4ClassTable[index].dataPtr );

	if (Falcon4ClassTable[index].dataType <= DTYPE_MIN || Falcon4ClassTable[index].dataType >= DTYPE_MAX)  // JB 010106 CTD sanity check
		return NULL; // JB 010106 CTD sanity check

	return  (FeatureClassDataType*) Falcon4ClassTable[index].dataPtr;
	}

int GetFeatureRepairTime (int index)
	{
	FeatureClassDataType*		fc;

	fc = GetFeatureClassData(index);
	if (!fc)
		return 0;
	return fc->RepairTime;
	}

/*
int GetFeatureHitChance (int id, int mt, int range, int hitflags)
	{
	FeatureClassDataType*	fc;
	int							i,wid,hc=0,bc=0;

	fc = GetFeatureClassData(id);
	if (!fc)
		return 0;
	for (i=0; i<HARDPOINT_MAX; i++)
		{
		wid = fc->Weapon[i];
		if (wid && fc->Weapons[i])
			{
			if (wid < 255 && GetWeaponRange(wid,mt) >= range)
				hc = GetWeaponHitChance(wid,mt);
			if (hc > bc)
				bc = hc;
			}
		}
	return bc;
	}

// Returns precalculated best hitchance
int GetAproxFeatureHitChance (int id, int mt, int range)
	{
	FeatureClassDataType*	fc;

	fc = GetFeatureClassData(id);
	if (fc && fc->Range[mt] >= range)
		{
		if (fc->Range[mt])
			return (int)(fc->HitChance[mt]*(1.5F - (float)(range/fc->Range[mt])));
		else
			return fc->HitChance[mt];
		}
	return 0;
	}

int CalculateFeatureHitChance (int id, int mt)
	{
	int							i,hc,bc=0;
	FeatureClassDataType*	fc;

	fc = GetFeatureClassData(id);
	if (!fc)
		return 0;
	for (i=0; i<HARDPOINT_MAX; i++)
		{
		if (fc->Weapon[i] && fc->Weapons[i])
			{
			hc = GetWeaponHitChance(fc->Weapon[i],mt);
			if (hc > bc)
				bc = hc;
			}
		}
	return bc;
	}

// Returns the precalculated strength at a given range
int GetAproxFeatureCombatStrength (int id, int mt, int range)
	{
	FeatureClassDataType*	fc;

	fc = GetFeatureClassData(id);
	if (fc && fc->Range[mt] >= range)
		return fc->Strength[mt];
	return 0;
	}

// Returns the exact strength at a given range
int GetFeatureCombatStrength (int id, int mt, int range)
	{
	int							i,str=0,bs=0,wid;
	FeatureClassDataType*	fc;

	fc = GetFeatureClassData(id);
	if (!fc)
		return 0;
	for (i=0; i<HARDPOINT_MAX; i++)
		{
		wid = fc->Weapon[i];
		if (wid && fc->Weapons[i])
			{
			if (wid < 255 && GetWeaponRange(wid,mt) >= range)
				str = GetWeaponScore(wid,mt,range);
			if (str > bs)
				bs = str;
			}
		}
	return bs;
	}

// Calculates the best strength possible against the passed movement type (ignores range)
int CalculateFeatureCombatStrength (int id, int mt)
	{
	int							i,str,bs=0;
	FeatureClassDataType*	fc;

	fc = GetFeatureClassData(id);
	if (!fc)
		return 0;
	for (i=0; i<HARDPOINT_MAX; i++)
		{
		if (fc->Weapon[i] && fc->Weapons[i])
			{
			str = GetWeaponScore(fc->Weapon[i],mt,0);
			if (str > bs)
				bs = str;
			}
		}
	return bs;
	}

// Returns the precalculated max range
int GetAproxFeatureRange (int id, int mt)
	{
	FeatureClassDataType*	fc;

	fc = GetFeatureClassData(id);
	if (!fc)
		return 0;
	return fc->Range[mt];
	}

// Returns current feature maximum range
int GetFeatureRange (int id, int mt)
	{
	int							i,wid,rng,br=0;
	FeatureClassDataType*	fc;

	fc = GetFeatureClassData(id);
	if (!fc)
		return 0;
	for (i=0; i<HARDPOINT_MAX; i++)
		{
		wid = fc->Weapon[i];
		if (wid && fc->Weapons[i])
			{
			if (wid < 255)
				rng = GetWeaponRange(wid,mt);
			if (rng > br)
				br = rng;
			}
		}
	return br;
	}

// Determines the Feature's max range (this is used to do the precalculation)
int CalculateFeatureRange (int id, int mt)
	{
	int							i,rng,br=0;
	FeatureClassDataType*	fc;

	fc = GetFeatureClassData(id);
	if (!fc)
		return 0;
	for (i=0; i<HARDPOINT_MAX; i++)
		{
		if (fc->Weapon[i] && fc->Weapons[i])
			{
			rng = GetWeaponRange(fc->Weapon[i], mt);
			if (rng > br)
				br = rng;
			}
		}
	return br;
	}
*/

int GetFeatureDetectionRange (int id, int mt)
	{
	FeatureClassDataType*	fc;

	fc = GetFeatureClassData(id);
	if (!fc)
		return 0;
	return fc->Detection[mt];
	}

/*
int GetBestFeatureWeapon(int id, uchar* dam, MoveType m, int range)
	{
	int			i,str,bs,w,bw;
	FeatureClassDataType*	fc;

	bw = bs = 0;
	fc = GetFeatureClassData(id);
	if (!fc)
		return 0;
	for (i=0; i<HARDPOINT_MAX; i++)
		{
		if (fc->Weapons[i] > 0 && fc->Weapon[i] < 1000 && fc->Weapon[i] > 0)
			{
			w = fc->Weapon[i];
			str = GetWeaponScore (w, dam, m, range);
			if (str > bs)
				{
				bw = w;
				bs = str;
				}
			}
		}
	return bw;
	}

*/
