#include <stdio.h>
#include <conio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <math.h>
#include "CmpGlobl.h"
#include "Campterr.h"
#include "Campcell.h"
#include "vutypes.h"
#include "entity.h"
#include "ASearch.h"
#include "Campaign.h"
#include "Campaign.h"
#include "Unit.h"
#include "vehicle.h"
#include "WeapList.h"

// ======================================
// Vehicle related stuff
// ======================================

char* GetVehicleName(VehicleID vid)
{
    VehicleClassDataType* vc;

    vc = (VehicleClassDataType*) Falcon4ClassTable[vid].dataPtr;

    if ( not vc)
        return "None";

    return vc->Name;
}

VehicleClassDataType* GetVehicleClassData(int index)
{
    return (VehicleClassDataType*) Falcon4ClassTable[index].dataPtr;
}

// Calculates hit chance of built in weapons. Estimates for loadable weapons are maximum
int CalculateVehicleHitChance(int id, int mt)
{
    int i, hc, bc = 0, wid, wl, j;
    VehicleClassDataType* vc;

    vc = GetVehicleClassData(id);

    if ( not vc)
        return 0;

    for (i = 0; i < HARDPOINT_MAX; i++)
    {
        wid = vc->Weapon[i];

        if (wid and vc->Weapons[i])
        {
            if (vc->Weapons[i] == 255)
            {
                // We've got to check every weapon in the list
                wl = wid;

                for (j = 0; j < MAX_WEAPONS_IN_LIST; j++)
                {
                    wid = GetListEntryWeapon(wl, j);

                    if (wid > 0)
                    {
                        hc = GetWeaponHitChance(wid, mt);

                        if (hc > bc)
                            bc = hc;
                    }
                }
            }
            else
            {
                hc = GetWeaponHitChance(wid, mt);

                if (hc > bc)
                    bc = hc;
            }
        }
    }

    return bc;
}

// Returns the precalculated average strength at a given range
int GetAproxVehicleCombatStrength(int id, int mt, int range)
{
    VehicleClassDataType* vc;

    vc = GetVehicleClassData(id);

    if (vc and vc->Range[mt] >= range)
        return vc->Strength[mt];

    return 0;
}

// Calculates the best strength possible (including hit chance) against the passed movement type (ignores range)
int CalculateVehicleCombatStrength(int id, int mt)
{
    int i, j, str, wid, wl, bs = 0;
    VehicleClassDataType* vc;

    vc = GetVehicleClassData(id);

    if ( not vc)
        return 0;

    for (i = 0; i < HARDPOINT_MAX; i++)
    {
        wid = vc->Weapon[i];

        if (wid and vc->Weapons[i])
        {
            if (vc->Weapons[i] == 255)
            {
                // Find the best weapon in the list
                wl = wid;

                for (j = 0; j < MAX_WEAPONS_IN_LIST; j++)
                {
                    wid = GetListEntryWeapon(wl, j);

                    if (wid > 0)
                    {
                        str = GetWeaponScore(wid, mt, 0);

                        if (str > bs)
                            bs = str;
                    }
                }
            }

            str = GetWeaponScore(wid, mt, 0);

            if (str > bs)
                bs = str;
        }
    }

    return bs;
}

// Returns the precalculated max range
int GetAproxVehicleRange(int id, int mt)
{
    VehicleClassDataType* vc;

    vc = GetVehicleClassData(id);

    if ( not vc)
        return 0;

    return vc->Range[mt];
}

// Determines the vehicle's max range (this is used to do the precalculation)
int CalculateVehicleRange(int id, int mt)
{
    int i, j, rng, wid, wl, br = 0;
    VehicleClassDataType* vc;

    vc = GetVehicleClassData(id);

    if ( not vc)
        return 0;

    for (i = 0; i < HARDPOINT_MAX; i++)
    {
        wid = vc->Weapon[i];

        if (wid and vc->Weapons[i])
        {
            if (vc->Weapons[i] == 255)
            {
                // We've got to check every weapon in the list
                wl = wid;

                for (j = 0; j < MAX_WEAPONS_IN_LIST; j++)
                {
                    wid = GetListEntryWeapon(wl, j);

                    if (wid > 0)
                    {
                        rng = GetWeaponRange(wid, mt);

                        if (rng > br)
                            br = rng;
                    }
                }
            }
            else
            {
                rng = GetWeaponRange(wid, mt);

                if (rng > br)
                    br = rng;
            }
        }
    }

    return br;
}

int GetVehicleDetectionRange(int id, int mt)
{
    VehicleClassDataType* vc;

    vc = (VehicleClassDataType*) Falcon4ClassTable[id].dataPtr;

    if ( not vc)
        return 0;

    return vc->Detection[mt];
}

int GetBestVehicleWeapon(int id, uchar* dam, MoveType m, int range, int *hard_point)
{
    int i, str, bs, w, ws, bw = -1, bhp = -1;
    VehicleClassDataType* vc;

    vc = (VehicleClassDataType*) Falcon4ClassTable[id].dataPtr;

    if ( not vc)
        return 0;

    bw = bs = 0;

    for (i = 0; i < HARDPOINT_MAX; i++)
    {
        w = vc->Weapon[i];
        ws = vc->Weapons[i];
        ShiAssert(ws < 255)

        if (w and ws)
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

    *hard_point = bhp;
    return bw;
}

int GetVehicleWeapon(int vid, int hp)
{
    VehicleClassDataType* vc;

    vc = (VehicleClassDataType*) Falcon4ClassTable[vid].dataPtr;
    ShiAssert(vc);
    return vc->Weapon[hp];
}

int GetVehicleWeapons(int vid, int hp)
{
    VehicleClassDataType* vc;

    vc = (VehicleClassDataType*) Falcon4ClassTable[vid].dataPtr;
    ShiAssert(vc);
    return vc->Weapons[hp];
}
