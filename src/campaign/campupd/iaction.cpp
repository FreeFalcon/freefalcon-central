///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <process.h>
#include "flight.h"
#include "CmpGlobl.h"
#include "F4VU.h"
#include "F4Find.h"
#include "falcmesg.h"
#include "F4Thread.h"
#include "CmpClass.h"
#include "CampTerr.h"
#include "Entity.h"
#include "Campaign.h"
#include "Team.h"
#include "Find.h"
#include "CmpEvent.h"
#include "CUIEvent.h"
#include "Weather.h"
#include "Name.h"
#include "MsgInc/RequestCampaignData.h"
#include "CampStr.h"
#include "MissEval.h"
#include "Pilot.h"
#include "CampMap.h"
#include "AIInput.h"
#include "Division.h"
#include "ThreadMgr.h"
#include "falcsess.h"
#include "Persist.h"
#include "RLE.h"
#include "PlayerOp.h"
#include "uicomms.h"
#include "classtbl.h"
#include "ui95/Chandler.h"
#include "Tacan.h"
#include "navsystem.h"
#include "rules.h"
#include "logbook.h"
#include "UserIDs.h"
#include "NavUnit.h"
#include "Dispcfg.h"
#include "ui/include/tac_class.h"
#include "ui/include/te_defs.h"
#include "iaction.h"
#include "ui_ia.h"
#include "TimerThread.h"

// 05/30/02 CLF -- Fixed unitialized local variables

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern VU_ID gSelectedFlightID;
extern VU_ID gActiveFlightID;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static struct ia_type
{
    char *name;
    instant_action_unit_type type;
    char stype;
    char sptype;

}
ia_air_objects[] =
{
    "an2", ia_an2, STYPE_UNIT_AIR_TRANSPORT, SPTYPE_AN2,
    "an24", ia_an24, STYPE_UNIT_AIR_TRANSPORT, SPTYPE_AN24,
    "c130", ia_c130, STYPE_UNIT_AIR_TRANSPORT, SPTYPE_C130,
    "il76", ia_il76, STYPE_UNIT_AIR_TRANSPORT, SPTYPE_IL76M,
    "y8", ia_y8, STYPE_UNIT_AIR_TRANSPORT, SPTYPE_Y8,
    "a10", ia_a10, STYPE_UNIT_ATTACK, SPTYPE_A10,
    "f4g", ia_f4g, STYPE_UNIT_ATTACK, SPTYPE_F4G,
    "fb111", ia_fb111, STYPE_UNIT_ATTACK, SPTYPE_FB111,
    "il28", ia_il28, STYPE_UNIT_ATTACK, SPTYPE_IL28,
    "mig27", ia_mig27, STYPE_UNIT_ATTACK, SPTYPE_MIG27,
    "su25", ia_su25, STYPE_UNIT_ATTACK, SPTYPE_SU25,
    "ah64", ia_ah64, STYPE_UNIT_ATTACK_HELO, SPTYPE_AH64,
    "ah64d", ia_ah64d, STYPE_UNIT_ATTACK_HELO, SPTYPE_AH64D,
    "ka50", ia_ka50, STYPE_UNIT_ATTACK_HELO, SPTYPE_KA50,
    "mi24", ia_mi24, STYPE_UNIT_ATTACK_HELO, SPTYPE_MI24,
    "a50", ia_a50, STYPE_UNIT_AWACS, SPTYPE_A50,
    "e3", ia_e3, STYPE_UNIT_AWACS, SPTYPE_E3,
    "b52g", ia_b52g, STYPE_UNIT_BOMBER, SPTYPE_B52G,
    "tu16", ia_tu16, STYPE_UNIT_BOMBER, SPTYPE_TU16,
    "ef111", ia_ef111, STYPE_UNIT_ECM, SPTYPE_EF111,
    "f14a", ia_f14a, STYPE_UNIT_FIGHTER, SPTYPE_F14A,
    "f15c", ia_f15c, STYPE_UNIT_FIGHTER, SPTYPE_F15C,
    "f4e", ia_f4e, STYPE_UNIT_FIGHTER, SPTYPE_F4E,
    "f5e", ia_f5e, STYPE_UNIT_FIGHTER, SPTYPE_F5E,
    "mig19", ia_mig19, STYPE_UNIT_FIGHTER, SPTYPE_MIG19,
    "mig21", ia_mig21, STYPE_UNIT_FIGHTER, SPTYPE_MIG21,
    "mig23", ia_mig23, STYPE_UNIT_FIGHTER, SPTYPE_MIG23MS,
    "mig25", ia_mig25, STYPE_UNIT_FIGHTER, SPTYPE_MIG25,
    "mig29", ia_mig29, STYPE_UNIT_FIGHTER, SPTYPE_MIG29,
    "su27", ia_su27, STYPE_UNIT_FIGHTER, SPTYPE_SU27,
    "f117", ia_f117, STYPE_UNIT_FIGHTER_BOMBER, SPTYPE_F117,
    "f15e", ia_f15e, STYPE_UNIT_FIGHTER_BOMBER, SPTYPE_F15E,
    "f16c", ia_f16c, STYPE_UNIT_FIGHTER_BOMBER, SPTYPE_F16C,
    "f18a", ia_f18a, STYPE_UNIT_FIGHTER_BOMBER, SPTYPE_F18A,
    "f18d", ia_f18d, STYPE_UNIT_FIGHTER_BOMBER, SPTYPE_F18D,
    "md500", ia_md500, STYPE_UNIT_RECON_HELO, SPTYPE_MD500,
    "oh58d", ia_oh58d, STYPE_UNIT_RECON_HELO, SPTYPE_OH58D,
    "il78", ia_il78, STYPE_UNIT_TANKER, SPTYPE_IL78,
    "kc10", ia_kc10, STYPE_UNIT_TANKER, SPTYPE_KC10,
    "kc135", ia_kc135, STYPE_UNIT_TANKER, SPTYPE_KC135,
    "tu16n", ia_tu16n, STYPE_UNIT_TANKER, SPTYPE_TU16N,
    "ch47", ia_ch47, STYPE_UNIT_TRANSPORT_HELO, SPTYPE_CH47,
    "uh1n", ia_uh1n, STYPE_UNIT_TRANSPORT_HELO, SPTYPE_UH1N,
    "uh60l", ia_uh60l, STYPE_UNIT_TRANSPORT_HELO, SPTYPE_UH60L,
    0, ia_unknown, 0, 0
},
ia_grnd_objects[] =
{
    "chinese_t80", ia_chinese_t80, STYPE_UNIT_ARMOR, SPTYPE_CHINESE_TYPE80,
    "chinese_t85", ia_chinese_t85, STYPE_UNIT_ARMOR, SPTYPE_CHINESE_TYPE85II,
    "chinese_t90", ia_chinese_t90, STYPE_UNIT_ARMOR, SPTYPE_CHINESE_TYPE90II,
    "chinese_sa6", ia_chinese_sa6, STYPE_UNIT_AIR_DEFENSE, SPTYPE_CHINESE_SA6,
    "chinese_zu23", ia_chinese_zu23, STYPE_UNIT_AIR_DEFENSE, SPTYPE_CHINESE_ZU23,
    "chinese_hq", ia_chinese_hq, STYPE_UNIT_HQ, SPTYPE_CHINESE_HQ,
    "chinese_inf", ia_chinese_inf, STYPE_UNIT_INFANTRY, SPTYPE_CHINESE_INF,
    "chinese_mech", ia_chinese_mech, STYPE_UNIT_MECHANIZED, SPTYPE_CHINESE_MECH,
    "chinese_sp", ia_chinese_sp, STYPE_UNIT_SP_ARTILLERY, SPTYPE_CHINESE_SP,
    "chinese_art", ia_chinese_art, STYPE_UNIT_TOWED_ARTILLERY, SPTYPE_CHINESE_ART,
    "dprk_aaa", ia_dprk_aaa, STYPE_UNIT_AIR_DEFENSE, SPTYPE_DPRK_AAA,
    "dprk_sa2", ia_drpk_sa2, STYPE_UNIT_AIR_DEFENSE, SPTYPE_DPRK_SA2,
    "dprk_sa3", ia_dprk_sa3, STYPE_UNIT_AIR_DEFENSE, SPTYPE_DPRK_SA3,
    "dprk_sa5", ia_dprk_sa5, STYPE_UNIT_AIR_DEFENSE, SPTYPE_DPRK_SA5,
    "dprk_airmobile", ia_dprk_airmobile, STYPE_UNIT_AIRMOBILE, SPTYPE_DPRK_AIR_MOBILE,
    "dprk_t55", ia_dprk_t55, STYPE_UNIT_ARMOR, SPTYPE_DPRK_T55,
    "dprk_t62", ia_dprk_t62, STYPE_UNIT_ARMOR, SPTYPE_DPRK_T62,
    "dprk_hq", ia_dprk_hq, STYPE_UNIT_HQ, SPTYPE_DPRK_HQ,
    "dprk_inf", ia_dprk_inf, STYPE_UNIT_INFANTRY, SPTYPE_DPRK_INF,
    "dprk_bmp1", ia_dprk_bmp1, STYPE_UNIT_MECHANIZED, SPTYPE_DPRK_BMP1,
    "dprk_bmp2", ia_dprk_bmp2, STYPE_UNIT_MECHANIZED, SPTYPE_DPRK_BMP2,
    "dprk_bm21", ia_dprk_bm21, STYPE_UNIT_ROCKET, SPTYPE_DPRK_BM21,
    "dprk_sp122", ia_dprk_sp122, STYPE_UNIT_SP_ARTILLERY, SPTYPE_DPRK_SP_122,
    "dprk_sp152", ia_dprk_sp152, STYPE_UNIT_SP_ARTILLERY, SPTYPE_DPRK_SP_152,
    "dprk_frog", ia_dprk_frog, STYPE_UNIT_SS_MISSILE, SPTYPE_DPRK_FROG,
    "dprk_scud", ia_dprk_scud, STYPE_UNIT_SS_MISSILE, SPTYPE_DPRK_SCUD,
    "dprk_tow_art", ia_dprk_tow_art, STYPE_UNIT_TOWED_ARTILLERY, SPTYPE_DPRK_TOW_ART,
    "rok_aaa", ia_rok_aaa, STYPE_UNIT_AIR_DEFENSE, SPTYPE_ROK_AAA,
    "rok_hawk", ia_rok_hawk, STYPE_UNIT_AIR_DEFENSE, SPTYPE_ROK_HAWK,
    "rok_nike", ia_rok_nike, STYPE_UNIT_AIR_DEFENSE, SPTYPE_ROK_NIKE,
    "rok_m48", ia_rok_m48, STYPE_UNIT_ARMOR, SPTYPE_ROK_M48,
    "rok_hq", ia_rok_hq, STYPE_UNIT_HQ, SPTYPE_ROK_HQ,
    "rok_inf", ia_rok_inf, STYPE_UNIT_INFANTRY, SPTYPE_ROK_INF,
    "rok_marine", ia_rok_marine, STYPE_UNIT_MARINE, SPTYPE_ROK_MARINE,
    "rok_m113", ia_rok_m113, STYPE_UNIT_MECHANIZED, SPTYPE_ROK_M113,
    "rok_sp", ia_rok_sp, STYPE_UNIT_SP_ARTILLERY, SPTYPE_ROK_SP,
    "rok_m198", ia_rok_m198, STYPE_UNIT_TOWED_ARTILLERY, SPTYPE_ROK_M198,
    "soviet_sa15", ia_soviet_sa15, STYPE_UNIT_AIR_DEFENSE, SPTYPE_SOVIET_SA15,
    "soviet_sa6", ia_soviet_sa6, STYPE_UNIT_AIR_DEFENSE, SPTYPE_SOVIET_SA6,
    "soviet_sa8", ia_soviet_sa8, STYPE_UNIT_AIR_DEFENSE, SPTYPE_SOVIET_SA8,
    "soviet_air", ia_soviet_air, STYPE_UNIT_AIRMOBILE, SPTYPE_SOVIET_AIR,
    "soviet_t72", ia_soviet_t72, STYPE_UNIT_ARMOR, SPTYPE_SOVIET_T72,
    "soviet_t80", ia_soviet_t80, STYPE_UNIT_ARMOR, SPTYPE_SOVIET_T80,
    "soviet_eng", ia_soviet_eng, STYPE_UNIT_ENGINEER, SPTYPE_SOVIET_ENG,
    "soviet_hq", ia_soviet_hq, STYPE_UNIT_HQ, SPTYPE_SOVIET_HQ,
    "soviet_inf", ia_soviet_inf, STYPE_UNIT_INFANTRY, SPTYPE_SOVIET_INF,
    "soviet_marine", ia_soviet_marine, STYPE_UNIT_MARINE, SPTYPE_SOVIET_MARINE,
    "soviet_mech", ia_soviet_mech, STYPE_UNIT_MECHANIZED, SPTYPE_SOVIET_MECH,
    "soviet_scud", ia_soviet_scud, STYPE_UNIT_SS_MISSILE, SPTYPE_SOVIET_SCUD,
    "soviet_frog7", ia_soviet_frog7, STYPE_UNIT_SS_MISSILE, SPTYPE_SOVIET_FROG7,
    "soviet_sp", ia_soviet_sp, STYPE_UNIT_SP_ARTILLERY, SPTYPE_SOVIET_SP,
    "soviet_sup", ia_soviet_sup, STYPE_UNIT_SUPPLY, SPTYPE_SOVIET_SUP,
    "soveit_bm21", ia_soviet_bm21, STYPE_UNIT_ROCKET, SPTYPE_SOVIET_BM21,
    "soviet_bm24", ia_soviet_bm24, STYPE_UNIT_ROCKET, SPTYPE_SOVIET_BM24,
    "soviet_bm9a52", ia_soviet_bm9a52, STYPE_UNIT_ROCKET, SPTYPE_SOVIET_BM9A52,
    "soviet_art", ia_soviet_art, STYPE_UNIT_TOWED_ARTILLERY, SPTYPE_SOVIET_ART,
    "us_patriot", ia_us_patriot, STYPE_UNIT_AIR_DEFENSE, SPTYPE_US_PATRIOT,
    "us_hawk", ia_us_hawk, STYPE_UNIT_AIR_DEFENSE, SPTYPE_US_HAWK,
    "us_air", ia_us_air, STYPE_UNIT_AIRMOBILE, SPTYPE_US_AIR,
    "us_m1", ia_us_m1, STYPE_UNIT_ARMOR, SPTYPE_US_M1,
    "us_m60", ia_us_m60, STYPE_UNIT_ARMOR, SPTYPE_US_M60,
    "us_cav", ia_us_cav, STYPE_UNIT_ARMORED_CAV, SPTYPE_US_CAV,
    "us_eng", ia_us_eng, STYPE_UNIT_ENGINEER, SPTYPE_US_ENG,
    "us_hq", ia_us_hq, STYPE_UNIT_HQ, SPTYPE_US_HQ,
    "us_inf", ia_us_inf, STYPE_UNIT_INFANTRY, SPTYPE_US_INF,
    "us_lav25", ia_us_lav25, STYPE_UNIT_MARINE, SPTYPE_US_LAV25,
    "us_m2", ia_us_m2, STYPE_UNIT_MECHANIZED, SPTYPE_US_M2,
    "us_mlrs", ia_us_mirs, STYPE_UNIT_ROCKET, SPTYPE_US_MLRS,
    "us_m109", ia_us_m109, STYPE_UNIT_SP_ARTILLERY, SPTYPE_US_M109,
    "us_sup", ia_us_sup, STYPE_UNIT_SUPPLY, SPTYPE_US_SUP,
    0, ia_unknown, 0, 0
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

long instant_action::start_time = 0;
float instant_action::start_x = 2379087.0f;
float instant_action::start_y = 1568557.0f;

int instant_action::current_wave = 0;
int instant_action::generic_skill = 0;
char instant_action::current_mode = 'f';
unsigned long instant_action::wave_time = 0;
int instant_action::wave_created = 0;

FlightClass *instant_action::player_flight = 0;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::set_start_time(long t)
{
    start_time = t;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

long instant_action::get_start_time(void)
{
    return start_time;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::set_campaign_time(void)
{
    SetTime(start_time * 1000);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::set_start_position(float x, float y)
{
    start_x = x;
    start_y = y;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::get_start_position(float &x, float &y)
{
    x = start_x;
    y = start_y;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::check_next_wave(void)
{
    int aircraft_left, count_battalions, count_flights;
    Unit unit;
    GridIndex px, py, ux, uy;

    if (wave_created)
    {
        count_flights = 0;
        count_battalions = 0;
        aircraft_left = 0;

        if (FalconLocalSession->GetPlayerEntity())
        {
            player_flight->GetLocation(&px, &py);
            {
                VuListIterator iter(AllRealList);
                unit = GetFirstUnit(&iter);

                while (unit)
                {
                    if ((unit->GetDomain() == DOMAIN_AIR) or (unit->GetDomain() == DOMAIN_LAND))
                    {
                        if (( not unit->IsDead()) and (unit->IsBattalion()))
                        {
                            unit->GetLocation(&ux, &uy);

                            if (DistSqu(px, py, ux, uy) > 60 * 60)
                            {
                                // MonoPrint ("IA Killing Unit %08x @%d\n", unit, distance);

                                unit->KillUnit();
                            }
                            else
                            {
                                count_battalions ++;
                            }
                        }

                        if (( not unit->IsDead()) and (unit->IsFlight()))
                        {
                            aircraft_left += unit->GetTotalVehicles();

                            unit->GetLocation(&ux, &uy);

                            if (DistSqu(px, py, ux, uy) > 60 * 60)
                            {
                                // MonoPrint ("IA Killing Unit %08x @%d\n", unit, distance);

                                unit->KillUnit();
                                count_flights ++;
                            }
                            else if (unit->IAKill())
                            {
                                count_flights ++;
                            }
                        }
                    }

                    unit = GetNextUnit(&iter);
                }
            }

            if (current_mode == 'm')
            {
                if (count_battalions < 12)
                {
                    create_more_stuff();
                }
            }
            else
            {
                if (count_battalions < 4)
                {
                    create_more_stuff();
                }

                // MonoPrint ("Flights %d\n", count_flights);

                if (((count_flights == 0) or ((wave_time) and (TheCampaign.CurrentTime > wave_time))) and (aircraft_left < 4))
                {
                    current_wave ++;

                    create_wave();
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::set_start_wave(int wave)
{
    ShiAssert(wave >= 0 and wave <= 4); // Since its used as sim skill it had better be this way...

    current_wave = wave;
    generic_skill = wave;

    // SCR 11/30/98
    // It would be nice to do this here (as well as the team color hammering)
    // but that would require the Campaign to be loaded sooner.  It almost worked
    // when it was changed to the top of InstantActionFlyCB(), but it asserted and
    // I didn't want to figure out if the assert was necessary or not.
    // SO, I hacked this into the Team load from file.
    // TeamInfo[2]->airExperience = 60 + 10 * wave;
    // TeamInfo[2]->airDefenseExperience = 60 + 10 * wave;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::set_start_mode(char ch)
{
    current_mode = ch;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::create_wave(void)
{
    ia_data
    data;

    int
    loop = 0,
    valid_data = 0,
    value = 0;

    char
    *ptr = NULL,
     *str = NULL,
      buffer[MAX_PATH] = {0};

    FILE
    *fp = NULL;

    ia_type
    *ia = NULL;

    wave_created = 0;
    wave_time = 0;

    sprintf(buffer, "%s\\%c%d.ia", FalconCampUserSaveDirectory, current_mode, current_wave);

    fp = fopen(buffer, "r");

    if ( not fp)
    {
        current_wave --;
        sprintf(buffer, "%s\\%c%d.ia", FalconCampUserSaveDirectory, current_mode, current_wave);

        fp = fopen(buffer, "r");

        if ( not fp)
        {
            // MonoPrint ("Cannot open %s for Instant Action Wave File", buffer);
            current_wave ++;
            return;
        }

        // MonoPrint ("Repeating %s for Instant Action Wave File", buffer);
    }
    else
    {
        // MonoPrint ("Loading %s for Instant Action Wave File", buffer);
    }

    data.distance = 0;
    data.aspect = 0;
    data.altitude = 0;
    data.type = ia_unknown;
    data.size = 0;
    data.side = 0;
    data.dumb = 0;
    data.skill = generic_skill;
    data.guns = 1;
    data.radar = 1;
    data.heat = 1;
    data.ground = 1;
    data.num_vector = 0;

    valid_data = 0;

    while (fgets(buffer, 100, fp))
    {
        str = buffer;

        while (*str)
        {
            if ((*str not_eq ' ') and (*str not_eq '\t'))
            {
                break;
            }

            str ++;
        }

        if ((*str >= '0') and (*str <= '9'))
        {
            value = atoi(str);

            while (*str)
            {
                if (*str == ' ')
                {
                    str ++;
                    break;
                }

                str ++;
            }

            ptr = str;

            while (*ptr)
            {
                if ((*ptr == '\n') or (*ptr == '\r') or (*ptr == ' ') or (*ptr == '\t'))
                {
                    *ptr = '\0';
                    break;
                }

                ptr ++;
            }

            if (stricmp(str, "minutes") == 0)
            {
                wave_time = TheCampaign.CurrentTime + value * 60 * 1000;
            }
            else if (stricmp(str, "km") == 0)
            {
                if (data.num_vector)
                {
                    data.v_dist[data.num_vector - 1] = (float)value;
                }
                else
                {
                    data.distance = (float)value;
                }
            }
            else if (stricmp(str, "deg") == 0)
            {
                data.aspect = (float)value;
            }
            else if (stricmp(str, "kts") == 0)
            {
                if (data.num_vector)
                {
                    data.v_kts[data.num_vector - 1] = (float)value;
                }
            }
            else if (stricmp(str, "feet") == 0)
            {
                if (data.num_vector)
                {
                    data.v_alt[data.num_vector - 1] = (float)value;
                }
                else
                {
                    data.altitude = (float)value;
                }
            }
            else if (stricmp(str, "vector") == 0)
            {
                if (data.num_vector < 10)
                {
                    data.num_vector ++;
                    data.vector[data.num_vector - 1] = (float)value;
                }
            }
            else
            {
                ia = 0;

                for (loop = 0; ia_air_objects[loop].name; loop ++)
                {
                    if (stricmp(str, ia_air_objects[loop].name) == 0)
                    {
                        ia = &ia_air_objects[loop];

                        if (valid_data)
                        {
                            create_unit(data);
                        }

                        valid_data = 1;

                        data.size = value;
                        data.type = ia->type;
                        data.distance = 0;
                        data.aspect = 0;
                        data.altitude = 0;
                        data.side = 0;
                        data.dumb = 0;
                        data.skill = generic_skill;
                        data.guns = 1;
                        data.radar = 1;
                        data.heat = 1;
                        data.ground = 1;
                        data.num_vector = 0;
                        break;
                    }
                }

                if ( not ia)
                {
                    valid_data = 0;
                    MonoPrint("Unknown %d:%s\n", value, str);
                }
            }
        }
        else
        {
            ptr = str;

            while (*ptr)
            {
                if ((*ptr == '\n') or (*ptr == '\r') or (*ptr == ' ') or (*ptr == '\t'))
                {
                    *ptr = '\0';
                    break;
                }

                ptr ++;
            }

            if ((*str == '#') or (*str == '\0'))
            {
                // Comment or blank line
            }
            else
            {
                if (stricmp(str, "allied") == 0)
                {
                    data.side = 1;
                }
                else if (stricmp(str, "enemy") == 0)
                {
                    data.side = 2;
                }
                else if (stricmp(str, "neutral") == 0)
                {
                    data.side = 0;
                }
                else if (stricmp(str, "killthis") == 0)
                {
                    data.kill = 1;
                }
                else if (stricmp(str, "guns") == 0)
                {
                    data.guns = 1;
                }
                else if (stricmp(str, "radar") == 0)
                {
                    data.radar = 1;
                }
                else if (stricmp(str, "heat") == 0)
                {
                    data.heat = 1;
                }
                else if (stricmp(str, "ground") == 0)
                {
                    data.ground = 1;
                }
                else if (stricmp(str, "noweapons") == 0)
                {
                    data.guns = 0;
                    data.radar = 0;
                    data.heat = 0;
                }
                else if (stricmp(str, "full") == 0)
                {
                    data.guns = 1;
                    data.radar = 1;
                    data.heat = 1;
                }
                else if (stricmp(str, "dumb") == 0)
                {
                    data.dumb = 1;
                    data.skill = 0;
                }
                else if (stricmp(str, "recruit") == 0)
                {
                    data.skill = 0;
                }
                else if (stricmp(str, "cadet") == 0)
                {
                    data.skill = 1;
                }
                else if (stricmp(str, "rookie") == 0)
                {
                    data.skill = 2;
                }
                else if (stricmp(str, "veteran") == 0)
                {
                    data.skill = 3;
                }
                else if (stricmp(str, "ace") == 0)
                {
                    data.skill = 4;
                }
                else
                {
                    ia = 0;

                    for (loop = 0; ia_grnd_objects[loop].name; loop ++)
                    {
                        if (stricmp(str, ia_grnd_objects[loop].name) == 0)
                        {
                            ia = &ia_grnd_objects[loop];

                            if (valid_data)
                            {
                                create_unit(data);
                            }

                            valid_data = 1;

                            data.size = value;
                            data.type = ia->type;
                            data.distance = 0;
                            data.aspect = 0;
                            data.altitude = 0;
                            data.side = 0;
                            data.kill = 0;
                            data.dumb = 0;
                            data.skill = generic_skill;
                            data.guns = 1;
                            data.radar = 1;
                            data.heat = 1;
                            data.ground = 1;
                            data.num_vector = 0;
                            break;
                        }
                    }

                    if ( not ia)
                    {
                        valid_data = 0;
                        MonoPrint("Unknown Command %s\n", str);
                    }
                }
            }
        }
    }

    if (valid_data)
    {
        create_unit(data);
    }

    fclose(fp);

    wave_created = 1;

    return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::create_player_flight(void)
{
    //Modified by TW, 2003-10-17
    //You can now configure your plane type and loadout using the files m.ia and f.ia
    int
    loop = 0,
    valid_data = 0,
    value = 0,
    tid,
    i,
    type,
    subtype,
    specific,
    num_vehicles;

    char
    *ptr = NULL,
     *str = NULL,
      buffer[MAX_PATH] = {0};

    FILE
    *fp = NULL;

    ia_type
    *ia = NULL;

    short
    x,
    y;


    VehicleClassDataType
    *vc;

    LoadoutStruct
    *loadout;


    loadout = new LoadoutStruct;

    sprintf(buffer, "%s\\%c.ia", FalconCampUserSaveDirectory, current_mode);
    fp = fopen(buffer, "r");

    if (fp)
    {
        // MonoPrint ("Loading %s for Instant Action player plane File", buffer);
        // We have the file - read plane and loadout
        while (fgets(buffer, 100, fp))
        {
            str = buffer;

            while (*str)
            {
                if ((*str not_eq ' ') and (*str not_eq '\t'))
                {
                    break;
                }

                str ++;
            }

            if ((*str >= '0') and (*str <= '9'))
            {
                value = atoi(str);

                while (*str)
                {
                    if (*str == ' ')
                    {
                        str ++;
                        break;
                    }

                    str ++;
                }

                ptr = str;

                while (*ptr)
                {
                    if ((*ptr == '\n') or (*ptr == '\r') or (*ptr == ' ') or (*ptr == '\t'))
                    {
                        *ptr = '\0';
                        break;
                    }

                    ptr ++;
                }

                //Setup plane
                if (stricmp(str, "type") == 0)
                {
                    type = (unsigned char) value;
                }
                else if (stricmp(str, "subtype") == 0)
                {
                    subtype = (unsigned char) value;
                }
                else if (stricmp(str, "specific") == 0)
                {
                    specific = (unsigned char) value;
                }

                //Setup loadout
                else if (stricmp(str, "w0") == 0)
                {
                    loadout->WeaponID[0] = (short) value;
                }
                else if (stricmp(str, "c0") == 0)
                {
                    loadout->WeaponCount[0] = (unsigned char) value;
                }

                else if (stricmp(str, "w1") == 0)
                {
                    loadout->WeaponID[1] = (short) value;
                }
                else if (stricmp(str, "c1") == 0)
                {
                    loadout->WeaponCount[1] = (unsigned char) value;
                }

                else if (stricmp(str, "w2") == 0)
                {
                    loadout->WeaponID[2] = (short) value;
                }
                else if (stricmp(str, "c2") == 0)
                {
                    loadout->WeaponCount[2] = (unsigned char) value;
                }

                else if (stricmp(str, "w3") == 0)
                {
                    loadout->WeaponID[3] = (short) value;
                }
                else if (stricmp(str, "c3") == 0)
                {
                    loadout->WeaponCount[3] = (unsigned char) value;
                }

                else if (stricmp(str, "w4") == 0)
                {
                    loadout->WeaponID[4] = (short) value;
                }
                else if (stricmp(str, "c4") == 0)
                {
                    loadout->WeaponCount[4] = (unsigned char) value;
                }

                else if (stricmp(str, "w5") == 0)
                {
                    loadout->WeaponID[5] = (short) value;
                }
                else if (stricmp(str, "c5") == 0)
                {
                    loadout->WeaponCount[5] = (unsigned char) value;
                }

                else if (stricmp(str, "w6") == 0)
                {
                    loadout->WeaponID[6] = (short) value;
                }
                else if (stricmp(str, "c6") == 0)
                {
                    loadout->WeaponCount[6] = (unsigned char) value;
                }

                else if (stricmp(str, "w7") == 0)
                {
                    loadout->WeaponID[7] = (short) value;
                }
                else if (stricmp(str, "c7") == 0)
                {
                    loadout->WeaponCount[7] = (unsigned char) value;
                }

                else if (stricmp(str, "w8") == 0)
                {
                    loadout->WeaponID[8] = (short) value;
                }
                else if (stricmp(str, "c8") == 0)
                {
                    loadout->WeaponCount[8] = (unsigned char) value;
                }

                else if (stricmp(str, "w9") == 0)
                {
                    loadout->WeaponID[9] = (short) value;
                }
                else if (stricmp(str, "c9") == 0)
                {
                    loadout->WeaponCount[9] = (unsigned char) value;
                }
            }
        }

        fclose(fp);
        tid = GetClassID
              (
                  DOMAIN_AIR,
                  CLASS_UNIT,
                  type,
                  subtype,
                  specific,
                  0,
                  0,
                  0
              );

    }
    else
    {
        // MonoPrint ("Cannot open %s for Instant Action player plane File", buffer);
        //We don't have the file - use default plane and loadout

        tid = GetClassID
              (
                  DOMAIN_AIR,
                  CLASS_UNIT,
                  TYPE_FLIGHT,
                  STYPE_UNIT_FIGHTER_BOMBER,
                  SPTYPE_F16C,
                  0,
                  0,
                  0
              );


        if (current_mode == 'm')
        {
            // Moving Mud Loadout

            loadout->WeaponID[0] = 60; // Guns
            loadout->WeaponCount[0] = 51;

            loadout->WeaponID[1] = 12; // AIM-9M
            loadout->WeaponCount[1] = 1;

            loadout->WeaponID[2] = 71; // LAU-3/A /HE
            loadout->WeaponCount[2] = 19;

            loadout->WeaponID[3] = 20; // AGM-65G
            loadout->WeaponCount[3] = 3;

            loadout->WeaponID[4] = 81; // CBU-52B/B
            loadout->WeaponCount[4] = 3;

            loadout->WeaponID[5] = 70; // ALQ-131
            loadout->WeaponCount[5] = 1;

            loadout->WeaponID[6] = 64; // GBU-10C/B
            loadout->WeaponCount[6] = 1;

            loadout->WeaponID[7] = 65; // BLU-107/B
            loadout->WeaponCount[7] = 1;

            loadout->WeaponID[8] = 5; // Mk-82
            loadout->WeaponCount[8] = 3;

            loadout->WeaponID[9] = 23; // AGM-88
            loadout->WeaponCount[9] = 1;
        }
        else
        {
            // Fighter Sweep Loadout

            loadout->WeaponID[0] = 60; // Guns
            loadout->WeaponCount[0] = 51;

            loadout->WeaponID[1] = 56; //AIM120
            loadout->WeaponID[9] = 56;
            loadout->WeaponCount[1] = 1;
            loadout->WeaponCount[9] = 1;

            loadout->WeaponID[2] = 12; //AIM9-M
            loadout->WeaponID[8] = 12;
            loadout->WeaponCount[2] = 1;
            loadout->WeaponCount[8] = 1;

            loadout->WeaponID[3] = 150; //TW: AIM-9X
            loadout->WeaponCount[3] = 1;
            loadout->WeaponID[7] = 150;
            loadout->WeaponCount[7] = 1;

            loadout->WeaponID[5] = 70; //Jammer Pod
            loadout->WeaponCount[5] = 1;

        }

    }

    if ( not tid)
    {
        MonoPrint("Cannot create F16C Flight\n");
        return;
    }

    tid += VU_LAST_ENTITY_TYPE;

    player_flight = NewFlight(tid, 0, 0);

    if ( not player_flight)
    {
        MonoPrint("Cannot create FlightClass object\n");
        return;
    }

    num_vehicles = 1;

    // sfr: xy order
    ::vector pos = { start_x, start_y };
    ConvertSimToGrid(&pos, &x, &y);
    //x = SimToGrid (start_y);
    //y = SimToGrid (start_x);

    player_flight->SetOwner(1);
    player_flight->SetLocation(x, y);
    player_flight->SetAltitude(10000);
    player_flight->SetUnitMission(AMIS_SWEEP);

    switch (num_vehicles)
    {
        case 1:
        {
            player_flight->SetNumVehicles(0, 1);
            break;
        }

        case 2:
        {
            player_flight->SetNumVehicles(0, 2);
            break;
        }

        case 3:
        {
            player_flight->SetNumVehicles(0, 2);
            player_flight->SetNumVehicles(1, 1);
            break;
        }

        case 4:
        {
            player_flight->SetNumVehicles(0, 2);
            player_flight->SetNumVehicles(1, 2);
            break;
        }
    }

    for (i = 0; i < PILOTS_PER_FLIGHT; i ++)
    {
        if (i < num_vehicles)
        {
            player_flight->plane_stats[i] = AIRCRAFT_AVAILABLE;
            player_flight->pilots[i] = 0;
            player_flight->player_slots[i] = PILOTS_PER_FLIGHT;
            player_flight->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLEANDOOB);
        }
        else
        {
            player_flight->plane_stats[i] = AIRCRAFT_NOT_ASSIGNED;
            player_flight->pilots[i] = NO_PILOT;
            player_flight->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLEANDOOB);
        }
    }

    player_flight->last_player_slot = PILOTS_PER_FLIGHT;

    // Name this flight
    vc = GetVehicleClassData(player_flight->GetVehicleID(0));
    player_flight->callsign_id = vc->CallsignIndex;
    GetCallsignID(&player_flight->callsign_id, &player_flight->callsign_num, vc->CallsignSlots);

    if (player_flight->callsign_num)
    {
        SetCallsignID(player_flight->callsign_id, player_flight->callsign_num);
    }

    player_flight->SetLoadout(loadout, 1);
    player_flight->SetFinal(1);

    vuDatabase->/*Quick*/Insert(player_flight);

    FalconLocalSession->SetPlayerFlight(player_flight);
    FalconLocalSession->SetAircraftNum(0);
    FalconLocalSession->SetPilotSlot(PILOTS_PER_FLIGHT);
    FalconLocalSession->DoFullUpdate();

    gSelectedFlightID = player_flight->Id();

    TheCampaign.MissionEvaluator->PreMissionEval(player_flight, FalconLocalSession->GetPilotSlot());

    wave_created = 0;
    /*
     int
     tid;

     short
     x,
     y;

     int
     i,
     num_vehicles;

     VehicleClassDataType
     *vc;

     LoadoutStruct
     *loadout;

     tid = GetClassID
     (
     DOMAIN_AIR,
     CLASS_UNIT,
     TYPE_FLIGHT,
     STYPE_UNIT_FIGHTER_BOMBER,
     SPTYPE_F16C,
     0,
     0,
     0
     );

     if ( not tid)
     {
     MonoPrint ("Cannot create F16C Flight\n");
     return;
     }

     tid += VU_LAST_ENTITY_TYPE;

     player_flight = NewFlight(tid, 0, 0);

     if ( not player_flight)
     {
     MonoPrint ("Cannot create FlightClass object\n");
     return;
     }

     num_vehicles = 1;

     x = SimToGrid (start_y);
     y = SimToGrid (start_x);

     player_flight->SetOwner (1);
     player_flight->SetLocation (x, y);
     player_flight->SetAltitude (10000);

     player_flight->SetUnitMission (AMIS_SWEEP);

     switch (num_vehicles)
     {
     case 1:
     {
     player_flight->SetNumVehicles (0, 1);
     break;
     }

     case 2:
     {
     player_flight->SetNumVehicles (0, 2);
     break;
     }

     case 3:
     {
     player_flight->SetNumVehicles (0, 2);
     player_flight->SetNumVehicles (1, 1);
     break;
     }

     case 4:
     {
     player_flight->SetNumVehicles (0, 2);
     player_flight->SetNumVehicles (1, 2);
     break;
     }
     }

     for (i = 0; i < PILOTS_PER_FLIGHT; i ++)
     {
     if (i < num_vehicles)
     {
     player_flight->plane_stats[i] = AIRCRAFT_AVAILABLE;
     player_flight->pilots[i] = 0;
     player_flight->player_slots[i] = PILOTS_PER_FLIGHT;
     player_flight->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[144].priority);
     // player_flight->MakeFlightDirty (DIRTY_PLANE_STATS, SEND_RELIABLEANDOOB);
     }
     else
     {
     player_flight->plane_stats[i] = AIRCRAFT_NOT_ASSIGNED;
     player_flight->pilots[i] = NO_PILOT;
     player_flight->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[145].priority);
     // player_flight->MakeFlightDirty (DIRTY_PLANE_STATS, SEND_RELIABLEANDOOB);
     }
     }

     player_flight->last_player_slot = PILOTS_PER_FLIGHT;

     // Name this flight
     vc = GetVehicleClassData (player_flight->GetVehicleID (0));

     player_flight->callsign_id = vc->CallsignIndex;

     GetCallsignID (&player_flight->callsign_id, &player_flight->callsign_num, vc->CallsignSlots);

     if (player_flight->callsign_num)
     {
     SetCallsignID (player_flight->callsign_id, player_flight->callsign_num);
     }

     // Load some weapons
     loadout = new LoadoutStruct;

     if (current_mode == 'm')
     {
     // Moving Mud Loadout

     loadout->WeaponID[0] = 60; // Guns
     loadout->WeaponCount[0] = 51;

     loadout->WeaponID[1] = 12;
     loadout->WeaponCount[1] = 1;

     loadout->WeaponID[2] = 20;
     loadout->WeaponCount[2] = 1;

     loadout->WeaponID[3] = 71;
     loadout->WeaponCount[3] = 19;

     loadout->WeaponID[4] = 81;
     loadout->WeaponCount[4] = 3;

     loadout->WeaponID[5] = 70;
     loadout->WeaponCount[5] = 1;

     loadout->WeaponID[6] = 5;
     loadout->WeaponCount[6] = 3;

     // loadout->WeaponID[7] = 74;
     // edg test dynamics of durandal
     loadout->WeaponID[7] = 65;
     loadout->WeaponCount[7] = 1;

     loadout->WeaponID[8] = 64;
     loadout->WeaponCount[8] = 1;

     loadout->WeaponID[9] = 23;
     loadout->WeaponCount[9] = 1;
     }
     else
     {
     // Fighter Sweep Loadout

     loadout->WeaponID[0] = 60; // Guns
     loadout->WeaponCount[0] = 51;

     loadout->WeaponID[1] = 56; //AIM120
     loadout->WeaponID[9] = 56;
     loadout->WeaponCount[1] = 1;
     loadout->WeaponCount[9] = 1;

     loadout->WeaponID[2] = 12; //AIM9
     loadout->WeaponID[8] = 12;
     loadout->WeaponCount[2] = 1;
     loadout->WeaponCount[8] = 1;

     loadout->WeaponID[3] = 150; //AIM-9X
     loadout->WeaponCount[3] = 1;
     loadout->WeaponID[7] = 150;
     loadout->WeaponCount[7] = 1;
    //MI we don't want AG loadout in Fighter Sweep
    #if 0
     loadout->WeaponID[4] = 5; //MK82
     loadout->WeaponCount[4] = 3;
    #else
     loadout->WeaponID[4] = 0;
     loadout->WeaponCount[4] = 0;
    #endif
     loadout->WeaponID[5] = 70; //Jammer Pod
     loadout->WeaponCount[5] = 1;
    //MI we don't want AG loadout in Fighter Sweep
    #if 0
     loadout->WeaponID[6] = 19; //Maverick
     loadout->WeaponCount[6] = 3;
    #else
     loadout->WeaponID[6] = 0;
     loadout->WeaponCount[6] = 0;
    #endif
     }

     player_flight->SetLoadout(loadout,1);
     player_flight->SetFinal (1);

     vuDatabase->QuickInsert (player_flight);

     FalconLocalSession->SetPlayerFlight (player_flight);
     FalconLocalSession->SetAircraftNum (0);
     FalconLocalSession->SetPilotSlot (PILOTS_PER_FLIGHT);
     FalconLocalSession->DoFullUpdate ();

     gSelectedFlightID = player_flight->Id ();

     TheCampaign.MissionEvaluator->PreMissionEval (player_flight,FalconLocalSession->GetPilotSlot());

     wave_created = 0;
    */

}

#define MAX_IA_WAYPOINTS 4

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::move_player_flight(void)
{
    static const float FLIGHT_PLAN_SPEED = 350.0f * KNOTS_TO_FTPSEC;

    float
    nx,
    ny,
    lx,
    ly;

    short
    x,
    y;

    WayPointClass
    *waypoint,
    *last_waypoint;

    int
    time;


    if ( not player_flight)
    {
        create_player_flight();
    }

    //edg: this next bit o stuff looks for nearest airbases and sets
    // the players waypoints to them
    Objective o;
    int numWaypoints, i;
    float wx[MAX_IA_WAYPOINTS], wy[MAX_IA_WAYPOINTS];
    float d[MAX_IA_WAYPOINTS];

    numWaypoints     = 0;
    d[0] = 1000.0f * NM_TO_FT;
    d[1] = 1000.0f * NM_TO_FT;
    d[2] = 1000.0f * NM_TO_FT;
    d[3] = 1000.0f * NM_TO_FT;

    // CLF
    memset(wx, 0, MAX_IA_WAYPOINTS * sizeof(float));
    memset(wy, 0, MAX_IA_WAYPOINTS * sizeof(float));

    {
        VuListIterator lit(AllCampList);
        o = (Objective)lit.GetFirst();

        while (o)
        {
            if (o->GetType() == TYPE_AIRBASE)
            {
                // manhatten dist
                nx = (float)fabs(o->XPos() - start_x);
                ny = (float)fabs(o->YPos() - start_y);

                if (nx > ny)
                    nx = nx + ny * 0.5f;
                else
                    nx = ny + nx * 0.5f;

                // find closest
                if (nx < d[0])
                {
                    d[3] = d[2];
                    wx[3] = wx[2];
                    wy[3] = wy[2];
                    d[2] = d[1];
                    wx[2] = wx[1];
                    wy[2] = wy[1];
                    d[1] = d[0];
                    wx[1] = wx[0];
                    wy[1] = wy[0];
                    d[0] = nx;
                    wx[0] = o->XPos();
                    wy[0] = o->YPos();

                    if (numWaypoints < MAX_IA_WAYPOINTS)
                        numWaypoints++;

                }
                else if (nx < d[1])
                {
                    d[3] = d[2];
                    wx[3] = wx[2];
                    wy[3] = wy[2];
                    d[2] = d[1];
                    wx[2] = wx[1];
                    wy[2] = wy[1];
                    d[1] = nx;
                    wx[1] = o->XPos();
                    wy[1] = o->YPos();

                    if (numWaypoints < MAX_IA_WAYPOINTS)
                        numWaypoints++;

                }
                else if (nx < d[2])
                {
                    d[3] = d[2];
                    wx[3] = wx[2];
                    wy[3] = wy[2];
                    d[2] = nx;
                    wx[2] = o->XPos();
                    wy[2] = o->YPos();

                    if (numWaypoints < MAX_IA_WAYPOINTS)
                        numWaypoints++;

                }
                else if (nx < d[3])
                {
                    d[3] = nx;
                    wx[3] = o->XPos();
                    wy[3] = o->YPos();

                    if (numWaypoints < MAX_IA_WAYPOINTS)
                        numWaypoints++;

                }
            }

            o = (Objective) lit.GetNext();
        }
    }

    // sfr: xy order
    ::vector pos = { start_x, start_y };
    ConvertSimToGrid(&pos, &x, &y);
    //x = SimToGrid (start_y);
    //y = SimToGrid (start_x);

    player_flight->SetLocation(x, y);
    player_flight->SetAltitude(10000);

    // Create Waypoints

    // first one is at player's start location.....
    time = start_time * 1000;

    nx = start_x;
    ny = start_y;

    waypoint = new WayPointClass;
    waypoint->SetLocation(nx, ny, -10000);
    waypoint->SetWPArrive(time);
    waypoint->SetWPDepartTime(time);
    waypoint->SetWPAction(WP_CA);

    player_flight->wp_list = waypoint;

    last_waypoint = waypoint;
    lx = nx;
    ly = ny;

    // now loop thru the number of located airbases we found....
    for (i = 0; i < numWaypoints; i++)
    {
        nx = wx[i];
        ny = wy[i];

        time += FloatToInt32((Distance(lx, ly, nx, ny)) / FLIGHT_PLAN_SPEED) * 1000;

        waypoint = new WayPointClass;
        waypoint->SetLocation(nx, ny, -10000);
        waypoint->SetWPArrive(time);
        waypoint->SetWPDepartTime(time);
        waypoint->SetWPAction(WP_CA);

        if (i == numWaypoints - 1)
        {
            waypoint->SetWPFlags(WPF_REPEAT_CONTINUOUS bitor WPF_REPEAT);
        }

        last_waypoint->SetNextWP(waypoint);

        last_waypoint = waypoint;
        lx = nx;
        ly = ny;
    }

    if (numWaypoints >= 2)
        player_flight->SetCurrentWaypoint(2);
    else if (numWaypoints > 0)
        player_flight->SetCurrentWaypoint(1);

    player_flight->SetLastDirection(0);

    player_flight->SetUnitLastMove(start_time * 1000);
    player_flight->SetFinal(1);

    FalconLocalSession->SetPlayerFlight(player_flight);
    FalconLocalSession->SetAircraftNum(0);
    FalconLocalSession->SetPilotSlot(PILOTS_PER_FLIGHT);
    FalconLocalSession->DoFullUpdate();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::create_unit(ia_data &data)
{
    if (data.type < ia_battalion)
    {
        create_flight(data);
    }
    else
    {
        create_battalion(data);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::create_flight(ia_data &data)
{
    int
    time,
    i,
    loop;

    int
    tid;

    float
    rad;

    GridIndex
    px,
    py,
    lx,
    ly,
    nx,
    ny,
    x,
    y;

    ia_type
    *ia;

    FlightClass
    *flight,
    *new_flight;

    PackageClass
    *new_package;

    WayPointClass
    *waypoint,
    *last_waypoint;

    VehicleClassDataType
    *vc;

    ia = 0;

    for (loop = 0; ia_air_objects[loop].type; loop ++)
    {
        if (data.type == ia_air_objects[loop].type)
        {
            ia = &ia_air_objects[loop];
            break;
        }
    }

    if ( not ia)
    {
        MonoPrint("Cannot find Aircraft Type in Instant Action Object Table (ia_air_objects)");
        return;
    }

    flight = FalconLocalSession->GetPlayerFlight();

    if ( not flight)
    {
        return;
    }

    flight->GetLocation(&px, &py);
    rad = (flight->GetLastDirection() * 45 + data.aspect) * DTR; // WARNING - Dodgy code from Kevin...

    MonoPrint("Create Flight %d,%d %d ", px, py, flight->GetLastDirection() * 45);

    x = (short)(px + FloatToInt32((float)sin(rad) * data.distance));
    y = (short)(py + FloatToInt32((float)cos(rad) * data.distance));

    tid = GetClassID
          (
              DOMAIN_AIR,
              CLASS_UNIT,
              TYPE_FLIGHT,
              ia->stype,
              ia->sptype,
              0,
              0,
              0
          );

    if ( not tid)
    {
        MonoPrint("Cannot create F16C Flight\n");
        return;
    }

    tid += VU_LAST_ENTITY_TYPE;

    MonoPrint("=> %d,%d (%d Skill)\n", x, y, data.skill);

    new_flight = NewFlight(tid, 0, 0);

    if ( not new_flight)
    {
        MonoPrint("Cannot create FlightClass object\n");
        return;
    }

    new_flight->SetOwner(data.side);
    new_flight->SetLocation(x, y);
    new_flight->SetAltitude(FloatToInt32(data.altitude));
    // new_flight->SetInPackage(1);

    if ((data.dumb) or ( not ((data.radar) or (data.heat))))
    {
        new_flight->SetUnitMission(AMIS_NONE);
    }
    else if (data.ground)
    {
        new_flight->SetUnitMission(AMIS_SAD);
    }
    else
    {
        new_flight->SetUnitMission(AMIS_SWEEP);
    }

    switch (data.size)
    {
        case 1:
        {
            new_flight->SetNumVehicles(0, 1);
            break;
        }

        case 2:
        {
            new_flight->SetNumVehicles(0, 2);
            break;
        }

        case 3:
        {
            new_flight->SetNumVehicles(0, 2);
            new_flight->SetNumVehicles(1, 1);
            break;
        }

        case 4:
        {
            new_flight->SetNumVehicles(0, 2);
            new_flight->SetNumVehicles(1, 2);
            break;
        }
    }

    for (i = 0; i < PILOTS_PER_FLIGHT; i ++)
    {
        if (i < data.size)
        {
            new_flight->plane_stats[i] = AIRCRAFT_AVAILABLE;
            new_flight->pilots[i] = data.skill;
            //new_flight->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[146].priority);
            new_flight->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLEANDOOB);
        }
        else
        {
            new_flight->plane_stats[i] = AIRCRAFT_NOT_ASSIGNED;
            new_flight->pilots[i] = NO_PILOT;
            //new_flight->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[147].priority);
            new_flight->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLEANDOOB);
        }
    }

    new_flight->last_player_slot = PILOTS_PER_FLIGHT;

    // Name this flight
    vc = GetVehicleClassData(new_flight->GetVehicleID(0));

    new_flight->callsign_id = vc->CallsignIndex;

    GetCallsignID(&new_flight->callsign_id, &new_flight->callsign_num, vc->CallsignSlots);

    if (new_flight->callsign_num)
    {
        SetCallsignID(new_flight->callsign_id, new_flight->callsign_num);
    }

    // Create Waypoints

    time = TheCampaign.CurrentTime;

    waypoint = new WayPointClass(x, y, FloatToInt32(data.altitude), 0, time, 0, WP_CA, 0);

    new_flight->wp_list = waypoint;

    last_waypoint = waypoint;
    lx = x;
    ly = y;

    nx = x;
    ny = y;

    if (data.num_vector)
    {
        for (loop = 0; loop < data.num_vector; loop ++)
        {
            rad = (flight->GetLastDirection() * 45 + data.vector[loop]) * DTR; // WARNING - Dodgy code from Kevin...

            nx += (short)FloatToInt32((float)sin(rad) * data.v_dist[loop]);
            ny += (short)FloatToInt32((float)cos(rad) * data.v_dist[loop]);

            waypoint = new WayPointClass(nx, ny, FloatToInt32(data.v_alt[loop]), 0, time, 0, WP_CA, 0);

            last_waypoint->SetNextWP(waypoint);

            last_waypoint = waypoint;
            lx = nx;
            ly = ny;
        }
    }
    else
    {
        nx = px;
        ny = py;

        waypoint = new WayPointClass(nx, ny, FloatToInt32(data.altitude), 0, time, 0, WP_CA, 0);

        last_waypoint->SetNextWP(waypoint);
        last_waypoint = waypoint;
    }

    waypoint = new WayPointClass(nx, ny, FloatToInt32(data.altitude), 0, time, 0, WP_CA, WPF_REPEAT_CONTINUOUS bitor WPF_TARGET);

    last_waypoint->SetNextWP(waypoint);

    new_flight->SetCurrentWaypoint(2);

    gActiveFlightID = new_flight->Id();

    SetWPTimes(new_flight->GetFirstUnitWP(), TheCampaign.CurrentTime, new_flight->GetCombatSpeed(), 0);

    // Load some weapons

    if (data.guns)
    {
        new_flight->LoadWeapons(NULL, DefaultDamageMods, Air, 98, 0, 0);
    }

    else if (data.heat)
    {
        new_flight->LoadWeapons(NULL, DefaultDamageMods, Air, 2, 0, WEAP_HEATSEEKER);
    }

    else if (data.radar)
    {
        new_flight->LoadWeapons(NULL, DefaultDamageMods, Air, 2, 0, WEAP_RADAR);
    }

    else if (data.ground)
    {
        new_flight->LoadWeapons(NULL, DefaultDamageMods, Air, 2, 0, WEAP_BAI_LOADOUT);
    }

    //if ( not (data.guns) and not (data.heat) and not (data.radar))
    else
    {
        new_flight->LoadWeapons(NULL, DefaultDamageMods, NoMove, 0, 0, 0);
    }

    new_flight->SetUnitLastMove(time);

    // if this is a kc10 and its allied, and we don't have a player_flight's package,
    // then create a package, and set the tanker stuff up.
    if ((data.type == ia_kc10) and (data.side == 1)) // allied kc10
    {
        if ( not player_flight->GetUnitPackage())
        {
            new_package = (Package) NewUnit
                          (
                              DOMAIN_AIR,
                              TYPE_PACKAGE,
                              0,
                              0,
                              NULL
                          );

            if ( not new_package)
            {
                new_flight->SetFinal(1);
                return;
            }

            new_package->GetMissionRequest()->targetID = FalconNullId;
            new_package->GetMissionRequest()->mission = 0;
            new_package->SetOwner(1);

            new_package->SetTanker(new_flight->Id());

            player_flight->SetPackage(new_package->Id());

            new_flight->SetUnitMission(AMIS_TANKER);
        }
        else
        {
#if VU_ALL_FILTERED
            new_package = (PackageClass *)player_flight->GetUnitPackage();
#else
            new_package = (PackageClass *) vuDatabase->Find(player_flight->GetUnitPackage());
#endif

            if (new_package)
            {
                new_package->SetTanker(new_flight->Id());
            }
        }
    }

    if (data.kill)
    {
        new_flight->SetIAKill(data.kill);
    }

    new_flight->SetNoAbort(TRUE);

    new_flight->SetFinal(1);

    vuDatabase->/*Quick*/Insert(new_flight);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void instant_action::create_battalion(ia_data &data)
{
    int
    i,
    time,
    loop;

    int
    tid;

    float
    rad;

    GridIndex
    px,
    py,
    x,
    y;

    ia_type
    *ia;

    FlightClass
    *flight;

    BattalionClass
    *new_battalion;

    Objective
    objective;

    UnitClassDataType *uc;
    VehicleClassDataType *vc;
    Falcon4EntityClassType *classPtr;

    ia = 0;

    for (loop = 0; ia_grnd_objects[loop].type; loop ++)
    {
        if (data.type == ia_grnd_objects[loop].type)
        {
            ia = &ia_grnd_objects[loop];
            break;
        }
    }

    if ( not ia)
    {
        MonoPrint("Cannot find Battalion Type in Instant Action Object Table (ia_grnd_objects)");
        return;
    }

    flight = FalconLocalSession->GetPlayerFlight();

    if ( not flight)
    {
        return;
    }

    flight->GetLocation(&px, &py);
    rad = (flight->GetLastDirection() * 45 + data.aspect) * DTR; // WARNING - Dodgy code from Kevin...

    x = (short)(px + FloatToInt32((float)sin(rad) * data.distance));
    y = (short)(py + FloatToInt32((float)cos(rad) * data.distance));

    // edg: for now completely abort if we're going on a water tile.
    // TODO: search a bit more for an appropriate tile
    if (GetCover(x, y) == Water)
    {
        //MonoPrint ("Cannot start BATTALION on Water x = %d, y = %d\n", x, y );
        return;
    }

    tid = GetClassID(
              DOMAIN_LAND,
              CLASS_UNIT,
              TYPE_BATTALION,
              ia->stype,
              ia->sptype,
              0,
              0,
              0
          );

    if ( not tid)
    {
        //MonoPrint ("Cannot have BATTALION of Type\n");
        return;
    }

    uc = (UnitClassDataType*) Falcon4ClassTable[tid].dataPtr;
    tid += VU_LAST_ENTITY_TYPE;

    new_battalion = NewBattalion(tid, NULL);

    // filter weapons
    new_battalion->SetUnitSupply(100);
    vc = GetVehicleClassData(uc->VehicleType[0]);

    for (i = 0; i < HARDPOINT_MAX; i++)
    {
        if (vc->Weapon[i])
        {
            classPtr = &(Falcon4ClassTable[GetWeaponDescriptionIndex(vc->Weapon[i])]);

            if ( not InstantActionSettings.SamSites and classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_MISSILE)
                new_battalion->SetUnitSupply(0);

            if ( not InstantActionSettings.AAASites and classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_GUN)
                new_battalion->SetUnitSupply(0);
        }
    }

    new_battalion->SetOwner(data.side);
    new_battalion->SetLocation(x, y);
    //new_battalion->SetInPackage(1);

    time = TheCampaign.CurrentTime;

    // find an objective nearby
    objective = FindNearestObjective(x, y, NULL, 100);
    ShiAssert(objective);

    if (objective)
    {
        // Fake battalions into thinking they've gotten to their destination
        objective->GetLocation(&px, &py);
        //DSP: I'm going to try making everyone think they are airdefense, since that is
        //the only thing that matters in instant action

        new_battalion->SetUnitOrders(GORD_AIRDEFENSE, objective->Id());

        if (new_battalion->GetUnitNormalRole() == GRO_AIRDEFENSE)
            new_battalion->SetUnitDestination(x, y);
        else if (rand() % 3)
            new_battalion->SetUnitDestination(x, y);

        /*
        if (new_battalion->GetUnitNormalRole() == GRO_AIRDEFENSE)
        {
         new_battalion->SetUnitOrders(GORD_AIRDEFENSE,objective->Id());
         new_battalion->SetUnitDestination (x, y);
        }
        else if (objective->GetTeam() == new_battalion->GetTeam())
         new_battalion->SetUnitOrders(GORD_DEFEND,objective->Id());
        else if (objective->GetTeam() not_eq new_battalion->GetTeam())
         new_battalion->SetUnitOrders(GORD_CAPTURE,objective->Id());
        if (rand()%3)
         new_battalion->SetUnitDestination (x, y);*/
    }

    new_battalion->BuildElements();

    new_battalion->SetUnitLastMove(start_time * 1000);

    new_battalion->SetFinal(1);

    vuDatabase->/*Quick*/Insert(new_battalion);

    if (new_battalion->GetRadarType() not_eq RDR_NO_RADAR)
    {
        new_battalion->SetSearchMode((unsigned char)(FEC_RADAR_SEARCH_1));//me123 + rand()%3));
        new_battalion->SetEmitting(1);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static const struct
{
    instant_action_unit_type type; // Instanct action record label
    float range; // Approximate intial range (km)
} groundStuff[] =
#if 1
{
    { ia_dprk_bmp1, 10.0f }, // Always
    { ia_dprk_sp122, 10.0f },
    { ia_dprk_hq, 10.0f },
    { ia_dprk_t55, 15.0f },

    { ia_dprk_sp152, 10.0f }, // Moving Mud Level 0 "Recruit"
    { ia_dprk_scud, 15.0f },
    { ia_dprk_tow_art, 10.0f },
    { ia_dprk_aaa, 10.0f },

    { ia_dprk_bmp2, 10.0f }, // Moving Mud Level 1  "Cadet"
    { ia_dprk_frog, 10.0f },
    { ia_drpk_sa2, 20.0f },
    { ia_dprk_t62, 15.0f },

    { ia_dprk_inf, 7.0f }, // Moving Mud Level 2 "Rookie"
    { ia_dprk_bm21, 10.0f },
    { ia_dprk_sa5, 40.0f },
    { ia_soviet_t72, 15.0f },

    { ia_dprk_sa3, 15.0f }, // Moving Mud Level 3 "Veteran"
    { ia_soviet_bm24, 10.0f },
    { ia_soviet_sa8, 12.0f },
    { ia_soviet_mech, 10.0f },

    { ia_chinese_zu23, 10.0f }, // Moving Mud Level 4 "Ace"
    { ia_chinese_sa6, 30.0f },
    { ia_soviet_t80, 10.0f },
    { ia_soviet_sa15, 10.0f },
};
#else
{
    { ia_dprk_aaa, 10.0f },
    { ia_dprk_sa5, 40.0f },
    { ia_drpk_sa2, 20.0f },
    { ia_dprk_inf, 20.0f },
    { ia_dprk_inf, 20.0f },
    { ia_dprk_inf, 20.0f },
    { ia_dprk_inf, 20.0f },
    { ia_soviet_sa15, 20.0f },
};
#endif
static const int groundStuffLen = sizeof(groundStuff) / sizeof(groundStuff[0]);


void instant_action::create_more_stuff(void)
{
    int
    thing,
    level,
    time;

    ia_data
    data;

    time = TheCampaign.CurrentTime - start_time * 1000;

    if (current_mode == 'm')
    {
        level = (current_wave + 2) * 4; // Pick our starting level
        level += time / 30000; // One new vehicle type every 30 seconds
    }
    else
    {
        level = (current_wave + 1) * 4; // In fighter sweep, we bias down one level for ground things
        level += time / 120000; // One new vehicle type very 2 minutes
    }

    if (level > groundStuffLen)
    {
        level = groundStuffLen;
    }

    // MonoPrint ("Moving Mud Level %d\n", level);

    thing = rand() % level;

    data.type = groundStuff[thing].type;
    data.distance = groundStuff[thing].range * (0.75f + 0.5f * rand() / (float)RAND_MAX);
    data.aspect = (float)(rand() % 120 - 60);
    data.side = 2;

    create_battalion(data);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int instant_action::is_fighter_sweep(void)
{
    return (current_mode == 'f');
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int instant_action::is_moving_mud(void)
{
    return (current_mode == 'm');
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
