// Weapon.cpp
//
// Deals with campaign and sim loadout information

#include "falclib.h"
#include "campweap.h"
#include "entity.h"
#include "WeapList.h"

#ifdef DAVE_DBG
#include "simdrive.h"
#endif

extern float LOWAIR_RANGE_MODIFIER;
extern short NumWeaponTypes;

// ===========================================================
// Weapon information functions
// ===========================================================

int GetWeaponStrength(int w)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);

    if (w < 0 or w >= NumWeaponTypes) return 0;

    return WeaponDataTable[w].Strength;
}

int GetWeaponRange(int w, int mt)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);

    if ((w < 0) or (w >= NumWeaponTypes))
        return 0;

    // if (
    // F4IsBadReadPtr(&(WeaponDataTable[w]), sizeof(WeaponClassDataType)) or
    // F4IsBadReadPtr(&(WeaponDataTable[w].HitChance[mt]), sizeof(uchar)))
    // JB 011205 (too much CPU) // JB 010331 CTD
    if (w < 0 or w >= NumWeaponTypes)
    {
        // JB 011205
        return 0;
    }

    if (WeaponDataTable[w].HitChance[mt] > 0)
    {
        // KCK Hack: If vs LowAir and weapon has an Air hit chance, scale range
        if (mt == LowAir and WeaponDataTable[w].HitChance[Air])
        {
            return (int)((WeaponDataTable[w].Range * LOWAIR_RANGE_MODIFIER) + 0.99F);
        }

        return WeaponDataTable[w].Range;
    }

    return 0;
}

int GetWeaponHitChance(int w, int mt)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);

    if (w < 0 or w >= NumWeaponTypes) return 0;

    return WeaponDataTable[w].HitChance[mt];
}

int GetWeaponHitChance(int w, int mt, int range)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);
    ShiAssert(mt >= 0 and mt < 8);

    if (w < 0 or w >= NumWeaponTypes) return 0;

    int wr;

    wr = GetWeaponRange(w, mt);

    if (wr < range)
        return 0;

    return FloatToInt32(WeaponDataTable[w].HitChance[mt] * (1.2F - ((float)(range + 1) / (wr + 1))) + 0.5F);
}

int GetWeaponHitChance(int w, int mt, int range, int wrange)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);

    if (w < 0 or w >= NumWeaponTypes) return 0;

    return FloatToInt32(WeaponDataTable[w].HitChance[mt] * (1.2F - ((float)(range + 1) / (wrange + 1))) + 0.5F);
}

int GetWeaponFireRate(int w)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);

    if (w < 0 or w >= NumWeaponTypes) return 0;

    return WeaponDataTable[w].FireRate;
}

int GetWeaponScore(int w, int mt, int range)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);

    if (w < 0 or w >= NumWeaponTypes) return 0;

    int wr = GetWeaponRange(w, mt) < range;

    if (wr)
        return 0;

    return (WeaponDataTable[w].Strength * WeaponDataTable[w].FireRate * GetWeaponHitChance(w, mt, range, wr)) / 100;
}

int GetWeaponScore(int w, int mt, int range, int wrange)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);

    if (w < 0 or w >= NumWeaponTypes) return 0;

    return (WeaponDataTable[w].Strength * WeaponDataTable[w].FireRate * GetWeaponHitChance(w, mt, range, wrange)) / 100;
}

int GetWeaponScore(int w, uchar* dam, int mt, int range)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);

    if (w < 0 or w >= NumWeaponTypes) return 0;

    if (GetWeaponRange(w, mt) < range)
        return 0;

    return (WeaponDataTable[w].Strength * WeaponDataTable[w].FireRate * dam[WeaponDataTable[w].DamageType] * GetWeaponHitChance(w, mt, range)) / 100;
}

int GetWeaponScore(int w, uchar* dam, int mt, int range, int wrange)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);

    if (w < 0 or w >= NumWeaponTypes) return 0;

    return (WeaponDataTable[w].Strength * WeaponDataTable[w].FireRate * dam[WeaponDataTable[w].DamageType] * GetWeaponHitChance(w, mt, range, wrange)) / 100;
}

int GetWeaponDamageType(int w)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);

    if (w < 0 or w >= NumWeaponTypes) return 0;

    return (int)(WeaponDataTable[w].DamageType);
}

int GetWeaponDescriptionIndex(int w)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);

    if (w < 0 or w >= NumWeaponTypes) return 0;

    return (int)(WeaponDataTable[w].Index);
}

int GetWeaponIdFromDescriptionIndex(int index)
{
    return ((int)Falcon4ClassTable[index].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType);
}

int GetWeaponFlags(int w)
{
    ShiAssert(w >= 0 and w < NumWeaponTypes);

    if (w < 0 or w >= NumWeaponTypes) return 0;

    return (int)(WeaponDataTable[w].Flags);
}

// ===================================
// Weapon List stuff
// ===================================

int GetListEntryWeapon(int list, int num)
{
    ShiAssert(list >= 0 and list < NumWeaponTypes);

    if (list < 0 or list >= NumWeaponTypes) return 0;

    return WeaponListDataTable[list].WeaponID[num];
}

int GetListEntryWeapons(int list, int num)
{
    ShiAssert(list >= 0 and list < NumWeaponTypes);

    if (list < 0 or list >= NumWeaponTypes) return 0;

#ifdef DEBUG

    // KCK HACK FOR BAD DATA
    if (WeaponListDataTable[list].WeaponID[num] and not WeaponListDataTable[list].Quantity[num])
        return 1;

#endif
    return WeaponListDataTable[list].Quantity[num];
}

char* GetListName(int list)
{
    ShiAssert(list >= 0 and list < NumWeaponTypes);

    if (list < 0 or list >= NumWeaponTypes) return 0;

    return WeaponListDataTable[list].Name;
}
