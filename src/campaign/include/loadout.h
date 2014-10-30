// Loadout.h
//

#ifndef LOADOUT_H
#define LOADOUT_H

// ============================
// Loadout functions
// ============================

int LoadWeapons(void* squadron, int vindex, uchar *dam, MoveType mt, int num, int type_flags, int guide_flags, short Weapon[HARDPOINT_MAX], uchar Weapons[HARDPOINT_MAX]);

void LoadvsAir(int vindex, short Weapon[HARDPOINT_MAX], uchar Weapons[HARDPOINT_MAX]);

struct LoadoutStruct
{

#ifdef USE_SH_POOLS
    /*
       public:
       void *operator new(size_t size) { return MemAllocPtr(pool,size,FALSE); };
       void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
          static void InitializeStorage() { pool = MemPoolInit( 0 ); };
          static void ReleaseStorage() { MemPoolFree( pool ); };
          static MEM_POOL pool;
    */
#endif

    short WeaponID[HARDPOINT_MAX];
    uchar WeaponCount[HARDPOINT_MAX];

    LoadoutStruct(void)
    {
        memset(WeaponID, 0, (sizeof(short) * HARDPOINT_MAX));
        memset(WeaponCount, 0, (sizeof(uchar) * HARDPOINT_MAX));
    }

    const LoadoutStruct bitand operator = (const LoadoutStruct &rhs)
    {
        if (&rhs not_eq this)
        {
            memcpy(WeaponID, rhs.WeaponID, (sizeof(short) * HARDPOINT_MAX));
            memcpy(WeaponCount, rhs.WeaponCount, (sizeof(uchar) * HARDPOINT_MAX));
        }

        return *this;
    }
    /*
     bool operator == (LoadoutStruct bitand rhs)
     {
     bool bResult = true;

     for (int i = 0; i < HARDPOINT_MAX and bResult; ++i)
     {
     bResult = (WeaponID[i] == rhs.WeaponID[i]) and (WeaponCount[i] == rhs.WeaponCount[i]);
     }

     return bResult;
     }

     bool operator not_eq (LoadoutStruct bitand rhs)
     {
     bool bResult = false;

     for (int i = 0; i < HARDPOINT_MAX and not bResult; ++i)
     {
     bResult = (WeaponID[i] not_eq rhs.WeaponID[i]) or (WeaponCount[i] not_eq rhs.WeaponCount[i]);
     }

     return bResult;
     }
    */
};

struct LoadoutArray
{
    LoadoutStruct Stores[5];
};

#endif
