// Loadout.cpp
//
// Deals with campaign and sim loadout information

#include <stdio.h>
#include <conio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <limits.h>

#include "falclib.h"
#include "CmpClass.h"
#include "vutypes.h"
#include "camplib.h"
#include "campweap.h"
#include "WeapList.h"
#include "Campaign.h" // 2001-09-18 M.N. TV-guided weapons
#include "entity.h"
#include "loadout.h"
#include "Squadron.h"
#include "FalcSess.h"


#define INFINITE_AI_AMMO 1 // With this defined, if an AI is out of ammo, they'll load 2 of the worst thing they have

#ifdef USE_SH_POOLS
// MEM_POOL LoadoutStruct::pool;
#endif

extern short NumWeaponTypes;

extern uchar DefaultDamageMods[OtherDam + 1];
//extern int GetTotalTimeOfDay();
//extern int GetTimeOfDay(int i);
//extern float GetGroundColoring(int c);

uchar AdjustedWeaponCount(int *total, uchar max_this_station, short wid);

int LoadWeapon(int hp, int last_hp, short wid, int to_load, int max, Squadron squadron, short Weapon[HARDPOINT_MAX], uchar Weapons[HARDPOINT_MAX], VehicleClassDataType* vc)
{
    if (wid < 0 or wid >= NumWeaponTypes)
        return 0;

    if (squadron and not squadron->GetUnitStores(wid))
    {
#ifndef INFINITE_AI_AMMO
        // Check for infinite weapons
        int index;
        UnitClassDataType *uc = squadron->GetUnitClassData();
        ShiAssert(uc);
        index = uc->SpecialIndex;

        if (wid not_eq SquadronStoresDataTable[index].infiniteAA and wid not_eq SquadronStoresDataTable[index].infiniteAG and wid not_eq SquadronStoresDataTable[index].infiniteGun)
            return 0;

#endif

        // If this is a multi-shot hp, only load one shot's worth
        // KCK NOTE: For non-visible (ie: internal) stores, load max
        if (max > 2)
        {
            if ( not (vc->VisibleFlags bitand (0x01 << hp)))
                to_load = max; // Bomb-bay or gun - So fill 'er up
            else
                max = to_load = WeaponDataTable[wid].FireRate; // Otherwise, load one shot's

            if (last_hp + 1 - hp not_eq hp)
                to_load *= 2; // Twice as many, if we've got an opposite hp
        }
    }
    else
    {
        // We have plenty of weapons.. check for special case crapola
        // Bomb-bay or gun - one shot is considered the whole thing, so adjust to_load
        if ( not (vc->VisibleFlags bitand (0x01 << hp)))
        {
            to_load += max - 1; // Adjust 1 pt worth of to_load to max;

            if (last_hp + 1 - hp not_eq hp and to_load > max)
                to_load += max - 1; // Symetrical - Adjust another point
        }
    }

    if (last_hp + 1 - hp == hp or to_load bitand 0x01)
    {
        // Not loading symetrically
        Weapon[hp] = wid;
        Weapons[hp] = AdjustedWeaponCount(&to_load, (uchar)max, Weapon[hp]);
    }
    else
    {
        // Load symetrically
        int this_load = to_load / 2;
        Weapon[last_hp + 1 - hp] = Weapon[hp] = wid;
        Weapons[last_hp + 1 - hp] = Weapons[hp] = AdjustedWeaponCount(&this_load, (uchar)max, Weapon[hp]);
        to_load = this_load * 2;
    }

    return to_load;
}

int WeaponLoadScore(int wid, int lw, uchar *dam, MoveType mt, int type_flags, int guide_flags, int randomize)
{
    int score;

    if ( not wid or wid < 0 or wid >= NumWeaponTypes)
        return 0;

    //LRKLUDGE
    //if (wid == 184)
    // wid = 185;

    score = GetWeaponScore(wid, dam, mt, 0);

    // 2002-03-24 MN Don't load Nukes by the AI
    if (WeaponDataTable[wid].DamageType == NuclearDam)
        score = 0;

    // score = FloatToInt32((GetWeaponRange(wid,mt)+1)/5.0F*score);

    // RV - Biker - Rework this later on
    switch (type_flags)
    {
        case WEAP_ECM:
            break;

        case WEAP_BAI_LOADOUT:
            break;

        case WEAP_DEAD_LOADOUT:
            break;

        case WEAP_LASER_POD:
            if ((type_flags bitand WEAP_LASER_POD) and (WeaponDataTable[wid].Flags bitand WEAP_RECON) and (WeaponDataTable[wid].GuidanceFlags bitand WEAP_LASER))
                score = 10000;
            else
                score = 0;

            return score;
            break;

        case WEAP_FAC_LOADOUT:
            if (WeaponDataTable[wid].Flags bitand WEAP_ROCKET_MARKER)
                score = 10000;
            else
                score = 0;

            return score;

        case WEAP_CHAFF_POD:
            if ((type_flags bitand WEAP_CHAFF_POD) and (WeaponDataTable[wid].Flags bitand WEAP_RECON) and (WeaponDataTable[wid].Flags bitand WEAP_ECM))
                score = 10000;
            else
                score = 0;

            return score;
            break;

        default:
            break;
    }



    if (type_flags and not (type_flags bitand WeaponDataTable[wid].Flags) and not (type_flags bitand WEAP_BAI_LOADOUT) and not (type_flags bitand WEAP_DEAD_LOADOUT))
        score = 0;

    if (type_flags and type_flags bitand WeaponDataTable[wid].Flags)
        score += 100; // Needed so we load non-combat type things

    if ((guide_flags bitand WEAP_GUIDED_MASK) and (guide_flags bitand WeaponDataTable[wid].GuidanceFlags) not_eq guide_flags)
        score = 0;

    if (guide_flags == WEAP_DUMB_ONLY and (WeaponDataTable[wid].GuidanceFlags bitand WEAP_GUIDED_MASK))
        score = 0;

    // 2002-01-26 ADDED BY S.G. Don't use HARMS if not requested...
    if ( not guide_flags and (WeaponDataTable[wid].GuidanceFlags bitand WEAP_ANTIRADATION))
        score = 0;

    // END OF ADDED SECTION 2002-01-26
    if (type_flags bitand WEAP_BAI_LOADOUT)
    {
        if (((WeaponDataTable[wid].GuidanceFlags bitand WEAP_LASER) and (wid not_eq 68 or wid not_eq 310)) or // 2002-01-24 MODIFIED BY S.G. Added () around the '&' statements since it has lower precedence than and 
            (WeaponDataTable[wid].GuidanceFlags bitand WEAP_RADAR) or
            (WeaponDataTable[wid].GuidanceFlags bitand WEAP_ANTIRADATION))
            score = 0;

        //Cobra Test
        if ((WeaponDataTable[wid].GuidanceFlags bitand WEAP_REAR_ASPECT) or (WeaponDataTable[wid].GuidanceFlags bitand WEAP_FRONT_ASPECT))
            score = 0;

        // RV - Biker - Check for PEN type weapons here also
        if (WeaponDataTable[wid].GuidanceFlags == WEAP_VISUALONLY and WeaponDataTable[wid].DamageType == PenetrationDam)
            score = 0;

        /*FILE *fp = fopen("BAI.log","a");
        if (fp)
        {
         fprintf(fp, "WID: %3d Flags: %x Score: %d\n", wid, WeaponDataTable[wid].GuidanceFlags, score);
         fclose(fp);
        }*/
    }

    // RV - Biker - Something special for DEAD -> rework LGB and JDAM
    if (type_flags bitand WEAP_DEAD_LOADOUT)
    {
        score = score * WeaponDataTable[wid].Range / 100;

        if ((WeaponDataTable[wid].GuidanceFlags bitand WEAP_LASER) or
            ( not (WeaponDataTable[wid].Flags bitand WEAP_CLUSTER) and (WeaponDataTable[wid].Flags bitand WEAP_BOMBGPS)))
            score = score / (WeaponDataTable[wid].Strength + 1) * 200;

        if (WeaponDataTable[wid].GuidanceFlags ==  WEAP_VISUALONLY and WeaponDataTable[wid].DamageType == PenetrationDam)
            score = 0;

        if (WeaponDataTable[wid].GuidanceFlags ==  WEAP_LASER and WeaponDataTable[wid].DamageType == PenetrationDam)
            score = 0;
    }

    //Cobra 11/23/04 Removed the random thing; not needed.
    //Cobra 12/27/04 Put it back because Jim didn't get random loads of A/A weapons ;)
    if (score > 0 and randomize and FalconLocalGame and FalconLocalGame->GetGameType() not_eq game_Dogfight)
    {
        // Add some randomness in this
        if (rand() % 2)
            score /= 4;

        if (wid == lw)
        {
            if (mt not_eq Air) // Keep air to ground weapons similar
                score *= 4;
            else // Keep air to air weapons different
                score /= 4;
        }
    }

    // 2001-09-18 M.N. Prevent TV-Guided weapons from being picked for night /late dawndusk missions
    // stops if TimeOfDayGeneral returns dawndusk
    if (WeaponDataTable[wid].GuidanceFlags == WEAP_TV)
    {
        CampaignTime now = TheCampaign.CurrentTime;

        if (TimeOfDayGeneral(now) < TOD_DAWNDUSK)
            score = 0;

        if (TimeOfDayGeneral(now) == TOD_DAWNDUSK)
            score /= 4;

    }

    // 2001-09-18 M.N.
    return score;
}

// Takes a damage modifier array, movement type and flags to determine which weapons to load
int LoadWeapons(void *squadron, int vindex, uchar *dam, MoveType mt, int num, int type_flags, int guide_flags, short Weapon[HARDPOINT_MAX], uchar Weapons[HARDPOINT_MAX])
{
    int i, hp, wl, score, bs, bw, wid, lhp, chp, lw = 0, sl = 0, tl = num, force_on_one = 0;
    VehicleClassDataType *vc;
    UnitClassDataType *uc = NULL;

    //int temp_flags = type_flags;
    //int temp_num = 1;

    vc = (VehicleClassDataType*) Falcon4ClassTable[vindex].dataPtr;

    if ( not vc)
        return 0;

    if (squadron)
        uc = ((Squadron)squadron)->GetUnitClassData();

    if (type_flags bitand WEAP_FORCE_ON_ONE)
    {
        // We want to place all our weapons on one (or two if even) hardpoints)
        type_flags and_eq compl WEAP_FORCE_ON_ONE;
        force_on_one = 1;
    }

    // Find our last and center hardpoint
    lhp = chp = 0;

    for (hp = 0; hp < HARDPOINT_MAX; hp++)
    {
        if (vc->Weapon[hp])
            lhp = hp;
    }

    // We have a centerline, and only need one of these
    if (num == 1 and (lhp bitand 0x01))
        chp = (lhp / 2) + 1;

    // Check if we want a symetric load
    if ( not (num bitand 0x01))
        sl = 1;

    for (hp = chp; hp <= lhp and num > 0; hp++)
    {
        // RV - Biker - Jammers now do overwrite AA and AG weapons
        //if ( not Weapon[hp] and ( not sl or not Weapon[lhp+1-hp])) // Only check for empty hard points
        if ( not Weapon[hp] and ( not sl or not Weapon[lhp + 1 - hp]) or ((type_flags bitand WEAP_ECM or type_flags bitand WEAP_LASER_POD) and not (WeaponDataTable[Weapon[hp]].Flags bitand (WEAP_FUEL bitor WEAP_RECON)))) // Only check for empty hard points
            //if ( not Weapon[hp] or (temp_flags bitand WEAP_LASER_POD))
        {
            if (vc->Weapons[hp] == 255) // This is a weapon list
            {
                wl = vc->Weapon[hp];

                for (i = 0, bs = INT_MIN, bw = -1, wid = -1; i < MAX_WEAPONS_IN_LIST and wid; i++)
                {
                    wid = GetListEntryWeapon(wl, i);
                    score = WeaponLoadScore(wid, lw, dam, mt, type_flags, guide_flags, TRUE);

                    // Better score for bomb-bays, essentially
                    ShiAssert(WeaponDataTable[wid].FireRate);

                    if ( not (vc->VisibleFlags bitand (0x01 << hp)) and WeaponDataTable[wid].FireRate)
                        score = score * vc->Weapons[hp] / WeaponDataTable[wid].FireRate;

                    if (score and uc)
                    {
                        if ( not ((Squadron)squadron)->GetUnitStores(wid))
                        {
#ifndef INFINITE_AI_AMMO

                            // Check for infinite weapons
                            if (wid not_eq SquadronStoresDataTable[uc->SpecialIndex].infiniteAA and wid not_eq SquadronStoresDataTable[uc->SpecialIndex].infiniteAG)
                                score = 0;

#else
                            score = -score; // The worst thing wins..
#endif
                        }
                        else if (((Squadron)squadron)->GetUnitStores(wid) < 100)
                        {
                            // Lower score of rare things
                            if (wid not_eq SquadronStoresDataTable[uc->SpecialIndex].infiniteAA and wid not_eq SquadronStoresDataTable[uc->SpecialIndex].infiniteAG)
                                score = max(((score * ((Squadron)squadron)->GetUnitStores(wid)) / 100), 1);
                        }
                    }

                    if (score and score > bs)
                    {
                        bs = score;
                        bw = i;
                    }
                }

                if (bs > INT_MIN)
                {
                    if (force_on_one)
                        num = LoadWeapon(hp, lhp, GetListEntryWeapon(wl, bw), num, num / (sl + 1), (Squadron)squadron, Weapon, Weapons, vc);
                    else
                        num = LoadWeapon(hp, lhp, GetListEntryWeapon(wl, bw), num, GetListEntryWeapons(wl, bw), (Squadron)squadron, Weapon, Weapons, vc);
                }
            }
            else
            {
                if (hp)
                {
                    if (WeaponLoadScore(vc->Weapon[hp], 0, dam, mt, type_flags, guide_flags, FALSE))
                    {
                        if (type_flags bitand WEAP_LASER_POD)
                        {
                            num = LoadWeapon(hp, hp, GetListEntryWeapon(wl, bw), 1, 1, (Squadron)squadron, Weapon, Weapons, vc);
                            return 0;
                        }
                        else
                        {
                            if (force_on_one)
                                num = LoadWeapon(hp, lhp, vc->Weapon[hp], num, num / (sl + 1), (Squadron)squadron, Weapon, Weapons, vc);
                            else
                                num = LoadWeapon(hp, lhp, vc->Weapon[hp], num, vc->Weapons[hp], (Squadron)squadron, Weapon, Weapons, vc);
                        }
                    }
                }
                else
                {
                    // This is a freebee - it's our gun
                    Weapon[hp] = vc->Weapon[hp];
                    Weapons[hp] = vc->Weapons[hp]; // Max loadable weapons
                }
            }
        }

#ifdef DEBUG

        if (Weapon[hp])
            ShiAssert(Weapons[hp]);

#endif

        if (hp and not lw and Weapon[hp])
            lw = Weapon[hp];

        // RV - Biker - Maybe need to rework this
        if (type_flags bitand WEAP_LASER_POD and num == 0)
            break;

        // We checked the center, now do a normal search
        if (chp)
            hp = chp = 0;
    }

    return tl - num;
}

uchar AdjustedWeaponCount(int *total, uchar max_this_station, short wid)
{
    int count;
    int rate = GetWeaponFireRate(wid);

    if (rate > max_this_station)
        rate = max_this_station;

    if (*total == 1 and max_this_station > 0)
    {
        count = rate; // Load one shot's worth
        *total -= 1;
    }
    // else if (max_this_station/rate > *total/2)
    // {
    // count = ((*total)*rate)/2; // Load 1/2 num shot's worth (we'll load the other half symetrically)
    // *total /= 2;
    // }
    else
    {
        count = max_this_station; // Load the maximum
        *total -= count;
    }

    return (uchar)count;
}

void LoadvsAir(int vindex, short Weapon[HARDPOINT_MAX], uchar Weapons[HARDPOINT_MAX])
{
    uchar* damageMods;

    // This give 100% damage vs most common types of damage.
    // WARNING: if this doesn't get what we want, we can enter 100% for everything
    damageMods = DefaultDamageMods;

    // Load jamming pod, if possible
    LoadWeapons(NULL, vindex, damageMods, NoMove, 1, WEAP_ECM, 0, Weapon, Weapons);

    // Load AA weapons on remaining slots
    LoadWeapons(NULL, vindex, DefaultDamageMods, Air, 99, 0, 0, Weapon, Weapons);
}


