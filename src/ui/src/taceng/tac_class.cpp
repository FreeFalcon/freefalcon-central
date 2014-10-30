///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Tactical Engagement - Robin Heydon
//
// Implements the user interface for the tactical engagement section
// of FreeFalcon
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "falclib.h"
#include "vu2.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "F4Error.h"
#include "F4Find.h"
//#include "sim/include/simbase.h"
#include "cmpclass.h"
#include "tac_class.h"
#include "te_defs.h"
#include "team.h"
#include "unit.h"
#include "team.h"
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "find.h"
#include "division.h"
#include "cmap.h"
#include "flight.h"
#include "campwp.h"
#include "Listadt.h"
#include "objectiv.h"
#include "Campaign.h"
#include "classtbl.h"
#include "falcsess.h"
#include "gps.h"
#include "teamdata.h"
#include "Dispcfg.h"
#include "msginc/senduimsg.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int get_tactical_number_of_teams(void);
int get_tactical_number_of_aircraft(int);
int get_tactical_number_of_f16s(int);
void update_team_victory_window(void);
void UpdateVCOptions(victory_condition *vc);

extern long ShowGameOverWindow;
extern char gUI_CampaignFile[];
extern _TCHAR gUI_ScenarioName[];
extern uchar gSelectedTeam;
extern C_Map
*gMapMgr;
extern C_TreeList
*gVCTree;

extern long gRefreshScoresList;
extern short InCleanup;
int string_compare_extensions(char*, char*);
extern FILE* OpenCampFile(char *filename, char *ext, char *mode);
extern void StartCampaignGame(int local, int game_type);
extern void CloseCampFile(FILE *fp);
C_Victory *MakeVCControl(victory_condition *vc);
void MakeTacEngScoreList();

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int tactical_mission::hide_enemy_on(void)
{
    return (TheCampaign.TE_flags bitand tf_hide_enemy) and TRUE;
}

int tactical_mission::lock_ato_on(void)
{
    return (TheCampaign.TE_flags bitand tf_lock_ato) and TRUE;
}

int tactical_mission::lock_oob_on(void)
{
    return (TheCampaign.TE_flags bitand tf_lock_oob) and TRUE;
}

int tactical_mission::start_paused_on(void)
{
    return (TheCampaign.TE_flags bitand tf_start_paused) and TRUE;
}

int tactical_mission::is_flag_on(long value)
{
    return (TheCampaign.TE_flags bitand value) and TRUE;
}

void tactical_mission::set_flag(long value)
{
    TheCampaign.TE_flags or_eq value;
}

void tactical_mission::clear_flag(long value)
{
    TheCampaign.TE_flags and_eq compl value;
}

tactical_mission::tactical_mission(char *the_filename)
{
    TheCampaign.TE_type = tt_unknown;

    filename = new char[strlen(the_filename) + 1];

    strcpy(filename, the_filename);

    is_new = FALSE;
    is_online = FALSE;

    number_teams = 0;
    memset(number_aircraft, 0, sizeof(number_aircraft));
    memset(number_players, 0, sizeof(number_players));
    memset(number_f16s, 0, sizeof(number_f16s));
    memset(team_name, 0, sizeof(team_name));
    memset(team_pts, 0, sizeof(team_pts));

    conditions = NULL;

    TheCampaign.TE_flags = 0;
    filter = vcf_all;
    team = 1;
    game_over = 0;

    points_required = 0;

    info_load(the_filename);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

tactical_mission::tactical_mission(void)
{
    TheCampaign.TE_type = tt_engagement;

    filename = NULL;

    is_new = FALSE;
    is_online = TRUE;

    number_teams = 0;
    memset(number_aircraft, 0, sizeof(number_aircraft));
    memset(number_players, 0, sizeof(number_players));
    memset(number_f16s, 0, sizeof(number_f16s));
    memset(team_name, 0, sizeof(team_name));
    memset(team_pts, 0, sizeof(team_pts));

    conditions = NULL;

    TheCampaign.TE_flags = 0;
    filter = vcf_all;
    team = 1;

    points_required = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

tactical_mission::~tactical_mission(void)
{
    if (filename)
    {
        delete filename;

        filename = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_mission::set_type(tactical_type new_type)
{
    TheCampaign.TE_type = new_type;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

tactical_type tactical_mission::get_type(void)
{
    return (tactical_type) TheCampaign.TE_type;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char *tactical_mission::get_title(void)
{
    static char
    buffer[100];

    char
    *end,
    *dst,
    *ptr;

    if (filename)
    {
        end = filename;
        ptr = filename;
        dst = buffer;

        while (*ptr)
        {
            *dst = *ptr;

            if (*ptr == '\\')
            {
                dst = buffer;
                end = NULL;
            }
            else if (*ptr == '.')
            {
                end = dst ++;
            }
            else
            {
                dst ++;
            }

            ptr ++;
        }

        *dst = '\0';

        if (end)
        {
            *end = '\0';
        }
    }
    else
    {
        strcpy(buffer, TheCampaign.SaveFile);
    }

    return buffer;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void tactical_mission::save_data(char *savefile)
{
    FILE
    *fp;

    VU_ID
    id;

    victory_condition
    *vc;

    int
    loop;

    if ((fp = OpenCampFile(savefile, "te", "wb")) == NULL)
        return;

    if (conditions)
    {
        fprintf(fp, ":Victory\n");

        victory_condition::enter_critical_section();

        vc = conditions;

        while (vc)
        {
            id = vc->get_vu_id();

            // team, type, vu_id, value, tolerance, points
            fprintf
            (
                fp,
                "%d %d %d %d %d %d %d\n",
                vc->get_team(),
                vc->get_type(),
                id.num_,
                id.creator_,
                vc->get_sub_objective(),
                vc->get_tolerance(),
                vc->get_points()
            );

            vc = vc->succ;
        }

        victory_condition::leave_critical_section();
    }

    if (points_required)
    {
        fprintf(fp, ":Required\n%d\n", points_required);
    }

    fprintf(fp, ":Teams\n%d\n", get_tactical_number_of_teams());

    for (loop = 1; loop < 8; loop ++)
    {
        if (TeamInfo[loop])
        {
            fprintf
            (
                fp,
                ":Team\n%d %d %d\n",
                loop,
                get_tactical_number_of_aircraft(loop),
                get_tactical_number_of_f16s(loop)
            );

            fprintf(fp, ":TeamName\n%s\n", TeamInfo[loop]->GetName());
            fprintf(fp, ":TeamFlag\n%d\n", TeamInfo[loop]->GetFlag());
        }

    }

    if (filename)
    {
        strcpy(gUI_CampaignFile, filename);
    }

    CloseCampFile(fp);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void tactical_mission::save(char *filename)
{
    long saveIP, saveIter;

    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
    {
        TheCampaign.SetCreatorIP(0);
        TheCampaign.SetCreationTime(0);
        TheCampaign.SetCreationIter(0);
        TheCampaign.SaveCampaign(game_TacticalEngagement, filename, 0);  // Save Normal
    }
    else
    {
        if (FalconLocalGame->IsLocal())
        {
            gCommsMgr->SaveStats();
            TheCampaign.SetCreationIter(TheCampaign.GetCreationIter() + 1);
            TheCampaign.SaveCampaign(game_TacticalEngagement, filename, 0);  // Save Normal

            if (gCommsMgr->Online())
            {
                // Send messages to remote players with new Iter Number
                // So they can save their stats bitand update Iter in their campaign
                gCommsMgr->UpdateGameIter();
            }
        }
        else
        {
            // This person is making a LOCAL copy of someone elses game...
            // we can save His stats... but remote people will be SOL if he
            // tries to make this a remote game for them
            saveIP = TheCampaign.GetCreatorIP();
            saveIter = TheCampaign.GetCreationIter();
            TheCampaign.SetCreatorIP(FalconLocalSessionId.creator_);
            TheCampaign.SetCreationIter(1);

            gCommsMgr->SaveStats();
            TheCampaign.SaveCampaign(game_TacticalEngagement, filename, 0);  // Save Normal
            TheCampaign.SetCreatorIP(saveIP);
            TheCampaign.SetCreationIter(saveIter);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void tactical_mission::process_load(char *data, int size, int)
{
    char
    *ptr = NULL,
     buffer[1000];

    int
    len = 0,
    team = 0,
    x = 0,
    y = 0,
    loop = 0;

    enum tokens
    {
        t_null,
        t_type,
        t_marque,
        t_victory_condition,
        t_flags,
        t_points_required,
        t_number_teams,
        t_team_info,
        t_team_name,
        t_team_flag
    }
    current_state = t_null;

    struct
    {
        char *str;
        tokens token;
    }
    token_str[] =
    {
        "type", t_type,
        "victory", t_victory_condition,
        "flags", t_flags,
        "required", t_points_required,
        "teams", t_number_teams,
        "team", t_team_info,
        "teamname", t_team_name,
        "teamflag", t_team_flag,
        NULL, t_null
    };

    while (size)
    {
        ptr = buffer;

        while (size)
        {
            *ptr = *data;

            if ((*ptr == '\r') or (*ptr == '\n'))
            {
                while ((size) and ((*data == '\n') or (*data == '\r') or (*data == '\t') or (*data == ' ')))
                {
                    data ++;
                    size --;
                }

                break;
            }

            ptr ++;
            data ++;
            size --;
        }

        *ptr = '\0';

        // Find the length of the input

        len = strlen(buffer);

        // while we have some characters and they are not "good" characters, just chop the string shorter.

        while
        (
            (len > 0) and 
            (
                (buffer[len - 1] == '\n') or
                (buffer[len - 1] == '\r') or
                (buffer[len - 1] == '\t') or
                (buffer[len - 1] == ' ')
            )
        )
        {
            buffer[len - 1] = '\0';
            len --;
        }

        // Ok, we are now stripped.

        // Is it a command we know about, if so, update the current_state.

        if (buffer[0] == ':')
        {
            current_state = t_null;

            for (loop = 0; token_str[loop].str; loop ++)
            {
                if (_stricmp(token_str[loop].str, &buffer[1]) == 0)
                {
                    current_state = token_str[loop].token;
                    break;
                }
            }
        }
        else if (buffer[0] not_eq '\0') // Otherwise, do we have something to set this state's value to.
        {
            switch (current_state)
            {
                case t_type:
                {
                    if (strnicmp(buffer, "Training", 8) == 0)
                    {
                        TheCampaign.TE_type = tt_training;
                    }
                    else if (strnicmp(buffer, "Engagement", 10) == 0)
                    {
                        TheCampaign.TE_type = tt_engagement;
                    }
                    else if (strnicmp(buffer, "Single", 6) == 0)
                    {
                        TheCampaign.TE_type = tt_single;
                    }
                    else if (strnicmp(buffer, "Load", 4) == 0)
                    {
                        //support old files
                        TheCampaign.TE_type = tt_engagement;
                    }
                    else
                    {
                        MonoPrint("Unknown Type: ");
                        MonoPrint(filename);
                        MonoPrint(" ");
                        MonoPrint(buffer);
                        MonoPrint("\n");
                    }

                    break;
                }

                case t_flags:
                {
                    TheCampaign.TE_flags = atoi(buffer);
                    TheCampaign.TE_flags and_eq compl tf_start_paused; // Don't set the paused flag
                    break;
                }

                case t_victory_condition:
                {
                    setup_victory_condition(buffer);
                    break;
                }

                case t_points_required:
                {
                    set_points_required(atoi(buffer));
                    break;
                }

                case t_number_teams:
                {
                    number_teams = atoi(buffer);
                    break;
                }

                case t_team_info:
                {
                    sscanf(buffer, "%d %d %d", &team, &x, &y);

                    number_aircraft[team] = x;
                    number_f16s[team] = y;
                    break;
                }

                case t_team_name:
                {
                    set_team_name(team, buffer);
                    break;
                }

                case t_team_flag:
                {
                    set_team_flag(team, atoi(buffer));
                    break;
                }

                default:
                {
                    break;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

char *tactical_mission::read_te_file(char *filename, int *size)
{
    FILE
    *fp;

    int
    str_len,
    num_files,
    offset;

    char
    name[100],
         te_filename[100],
         *ext,
         *src,
         *dst,
         *ptr;

    ptr = filename;
    ext = ptr;
    dst = te_filename;
    src = NULL;

    while (*ptr)
    {
        *dst ++ = *ptr;

        if (*ptr == '.')
        {
            src = dst;
            ext = ptr + 1;
        }

        if (*ptr == '\\')
        {
            dst = te_filename;
            src = NULL;
        }

        ptr ++;
    }

    *dst = '\0';

    if (src)
    {
        *src ++ = 't';
        *src ++ = 'e';
        *src ++ = '\0';
    }

    if ((stricmp(ext, "tac") == 0) or (stricmp(ext, "trn") == 0))
    {
        //MonoPrint ("Extracting TE File from %s\n", filename);

        fp = fopen(filename, "rb");

        if (fp)
        {
            fread(&offset, 4, 1, fp);

            fseek(fp, offset, 0);

            fread(&num_files, 4, 1, fp);

            while (num_files)
            {
                fread(&str_len, 1, 1, fp);
                str_len and_eq 0xff;

                fread(name, str_len, 1, fp);
                \
                name[str_len] = '\0';

                if (string_compare_extensions(name, te_filename) == 0)
                {
                    fread(&offset, 4, 1, fp);
                    fread(size, 4, 1, fp);

                    fseek(fp, offset, 0);

                    ptr = new char[*size];

                    fread(ptr, *size, 1, fp);

                    fclose(fp);

                    return ptr;
                }
                else
                {
                    fseek(fp, 8, 1);  // seek relative
                }

                num_files --;
            }
        }

        return NULL;
    }
    else
    {
        fp = fopen(filename, "rb");

        if (fp)
        {
            fseek(fp, 0, 2);  // seek end

            *size = ftell(fp);

            fseek(fp, 0, 0);  // seek start

            ptr = new char[*size];

            fread(ptr, *size, 1, fp);

            fclose(fp);

            return ptr;
        }

        return NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void tactical_mission::info_load(char *the_filename)
{
    int
    size;

    char
    *data;

    if (strcmp(filename, the_filename) not_eq 0)
    {
        delete(filename);

        filename = new char[strlen(the_filename)];

        strcpy(filename, the_filename);
    }

    data = read_te_file(filename, &size);

    if ( not data)
    {
        MonoPrint("Cannot open a file we just decided existed\n");

        return;
    }

    process_load(data, size, FALSE);

    delete data;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void tactical_mission::load(void)
{
    int
    size;

    char
    *ptr,
    *ext,
    *data,
    *name;

    if ( not is_online)
    {
        data = read_te_file(filename, &size);

        //MonoPrint ("Tactical_Mission::Load %s\n", filename);

        if ( not data)
        {
            MonoPrint("Cannot open a file we just decided existed\n");

            return;
        }

        name = filename;

        ptr = filename;

        while (*ptr)
        {
            if (*ptr == '\\')
            {
                name = &ptr[1];
            }

            ptr ++;
        }

        ptr = gUI_CampaignFile;

        ext = NULL;

        while (*name)
        {
            if (*name == '.')
            {
                ext = ptr;
            }

            *ptr = *name;

            name ++;
            ptr ++;
        }

        *ptr = '\0';

        if (ext)
            *ext = '\0';

        ShowGameOverWindow = 0;

        _tcscpy(gUI_ScenarioName, get_title());

        FalconLocalSession->SetCountry(static_cast<uchar>(gSelectedTeam));

        SendMessage(FalconDisplay.appWin, FM_LOAD_CAMPAIGN, 0, game_TacticalEngagement);

        /* victory_condition::enter_critical_section ();

         while (conditions)
         {
         delete conditions;
         }

         conditions = NULL;

         victory_condition::leave_critical_section ();

         process_load (data, size, TRUE);
        */
        delete data;
    }
    else
    {
        FalconLocalSession->SetCountry(gSelectedTeam);
        StartCampaignGame(0, game_TacticalEngagement);
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void tactical_mission::preload(void)
{
    char
    *ext,
    *ptr,
    *name;

    if (this)
    {
        name = filename;

        ptr = filename;

        while (*ptr)
        {
            if (*ptr == '\\')
            {
                name = &ptr[1];
            }

            ptr ++;
        }

        ptr = gUI_CampaignFile;

        ext = ptr;

        while (*name)
        {
            if (*name == '.')
            {
                ext = ptr;
            }

            *ptr = *name;

            name ++;
            ptr ++;
        }

        *ext = '\0';
        *ptr = '\0';

        _tcscpy(gUI_ScenarioName, get_title());
    }
    else
    {
        strcpy(gUI_CampaignFile, "te_new");
        _tcscpy(gUI_ScenarioName, "te_new");
    }

    TheCampaign.Suspend();

    TheCampaign.LoadScenarioStats(game_TacticalEngagement, gUI_CampaignFile);

    // StartReadCampFile (game_TacticalEngagement, gUI_CampaignFile);
    //
    // if ( not LoadTeams (gUI_CampaignFile))
    // {
    // AddNewTeams (Neutral);
    // }
    //
    // EndReadCampFile ();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void tactical_mission::new_setup(void)
{
    //MonoPrint ("Tactical_Mission::New Setup\n");

    strcpy(gUI_CampaignFile, "te_new");

    is_new = TRUE;

    _tcscpy(gUI_ScenarioName, get_title());

    // FalconLocalSession->SetCountry (team);

    SendMessage(gMainHandler->GetAppWnd(), FM_LOAD_CAMPAIGN, 0, game_TacticalEngagement);

    //MonoPrint ("Tactical_Mission::New Setup\n");
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void tactical_mission::revert(void)
{
    //MonoPrint ("Tactical_Mission::Revert %s\n", filename);

    // HACK - Robin (unnessary hack)
    //FalconLocalSession->SetCountry(gSelectedTeam);
    SendMessage(gMainHandler->GetAppWnd(), FM_LOAD_CAMPAIGN, 0, game_TacticalEngagement);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int tactical_mission::get_number_of_teams(void)
{
    return number_teams;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int tactical_mission::get_number_of_aircraft(int team)
{
    return number_aircraft[team];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int tactical_mission::get_number_of_players(int team)
{
    return number_players[team];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int tactical_mission::get_number_of_f16s(int team)
{
    return number_f16s[team];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char *tactical_mission::get_team_name(int team)
{
    return team_name[team];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_mission::set_team_name(int team, char *name)
{
    if (team_name[team])
    {
        delete team_name[team];

        team_name[team] = NULL;
    }

    team_name[team] = new char [strlen(name) + 1];

    strcpy(team_name[team], name);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int tactical_mission::get_team_flag(int team)
{
    return team_flag[team];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_mission::set_team_flag(int team, int flag)
{
    team_flag[team] = static_cast<char>(flag);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_mission::setup_victory_condition(char *buffer)
{
    victory_type
    type;

    TREELIST
    *item;

    int
    team,
    vu_id_1,
    vu_id_2,
    value,
    tolerance,
    points;

    VU_ID
    id;

    victory_condition
    *vc;

    // team, type, vu_id, value, tolerance, points
    sscanf
    (
        buffer,
        "%d %d %d %d %d %d %d",
        &team,
        &type,
        &vu_id_1,
        &vu_id_2,
        &value,
        &tolerance,
        &points
    );

    id.num_ = vu_id_1;
    id.creator_ = vu_id_2;

    victory_condition::enter_critical_section();

    vc = new victory_condition(this);

    vc->set_team(team);
    vc->set_type(type);
    vc->set_vu_id(id);
    vc->set_sub_objective(value);
    vc->set_tolerance(tolerance);
    vc->set_points(points);

    vc->control = MakeVCControl(vc);

    if (gVCTree)
    {
        item = gVCTree->CreateItem(vc->get_number(), C_TYPE_ITEM, vc->control);

        if (item)
        {
            gVCTree->AddItem(gVCTree->GetRoot(), item);
            ((C_Victory*) vc->control)->SetOwner(item);
            vc->control->SetReady(1);
            vc->control->SetClient(gVCTree->GetClient());
            vc->control->SetParent(gVCTree->Parent_);
            vc->control->SetSubParents(gVCTree->Parent_);
            gVCTree->RecalcSize();

            if (gVCTree->Parent_)
                gVCTree->Parent_->RefreshClient(gVCTree->GetClient());
        }
    }

    if (gMapMgr)
    {
        gMapMgr->AddVC(vc);
    }

    UpdateVCOptions(vc);

    victory_condition::leave_critical_section();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int test_filter(victory_condition *vc, victory_condition_filter filter, int team)
{
    victory_condition::enter_critical_section();

    switch (filter)
    {
        case vcf_all:
        {
            victory_condition::leave_critical_section();

            return TRUE;
        }

        case vcf_team:
        {
            if (team == vc->get_team())
            {
                victory_condition::leave_critical_section();

                return TRUE;
            }

            break;
        }

        case vcf_all_achieved:
        {
            if (vc->get_active())
            {
                victory_condition::leave_critical_section();

                return TRUE;
            }

            break;
        }

        case vcf_all_remaining:
        {
            if ( not vc->get_active())
            {
                victory_condition::leave_critical_section();

                return TRUE;
            }

            break;
        }

        case vcf_team_achieved:
        {
            if
            (
                (team == vc->get_team()) and 
                (vc->get_active())
            )
            {
                victory_condition::leave_critical_section();

                return TRUE;
            }

            break;
        }

        case vcf_team_remaining:
        {
            if
            (
                (team == vc->get_team()) and 
                ( not vc->get_active())
            )
            {
                victory_condition::leave_critical_section();

                return TRUE;
            }

            break;
        }
    }

    victory_condition::leave_critical_section();

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_mission::set_victory_condition_filter(victory_condition_filter new_filter)
{
    filter = new_filter;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_mission::set_victory_condition_team_filter(int new_team)
{
    team = new_team;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

victory_condition *tactical_mission::get_first_victory_condition(void)
{
    victory_condition::enter_critical_section();

    current_vc = conditions;

    while (current_vc)
    {
        if (test_filter(current_vc, filter, team))
        {
            break;
        }

        current_vc = current_vc->succ;
    }

    victory_condition::leave_critical_section();

    return current_vc;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

victory_condition *tactical_mission::get_next_victory_condition(void)
{
    victory_condition::enter_critical_section();

    current_vc = current_vc->succ;

    while (current_vc)
    {
        if (test_filter(current_vc, filter, team))
        {
            break;
        }

        current_vc = current_vc->succ;
    }

    victory_condition::leave_critical_section();

    return current_vc;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

victory_condition *tactical_mission::get_first_unfiltered_victory_condition(void)
{
    victory_condition::enter_critical_section();

    current_vc = conditions;

    victory_condition::leave_critical_section();

    return current_vc;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

victory_condition *tactical_mission::get_next_unfiltered_victory_condition(void)
{
    victory_condition::enter_critical_section();

    current_vc = current_vc->succ;

    victory_condition::leave_critical_section();

    return current_vc;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_mission::set_points_required(int value)
{
    points_required = value;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int tactical_mission::get_points_required(void)
{
    return points_required;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_mission::evaluate_victory_conditions(void)
{
    UISendMsg *vcdone;
    victory_condition
    *vc;

    int
    count,
    per,
    old_active,
    changed = 0;

    Objective
    objective;

    Unit
    unit;

    victory_condition::enter_critical_section();

    vc = conditions;

    count = 1;

    while (vc)
    {
        if ( not vc->active)
        {
            old_active = vc->active;

            switch (vc->type)
            {
                case vt_occupy:
                {
                    objective = (Objective) FindEntity(vc->id);

                    if (objective and objective->IsObjective())
                    {
                        if (vc->team == objective->GetOwner())
                            vc->active = TRUE;

                        //else
                        // vc->active = FALSE;
                    }

                    break;
                }

                case vt_destroy:
                {
                    objective = (Objective) FindEntity(vc->id);

                    if (objective and objective->IsObjective())
                    {
                        int i;
                        int classID;
                        uchar origtype;
                        ObjClassDataType* oc;

                        oc = objective->GetObjectiveClassData();

                        if (oc)
                        {
                            classID = objective->GetFeatureID(vc->feature_id);
                            origtype = Falcon4ClassTable[classID].vuClassData.classInfo_[VU_TYPE];

                            for (i = 0; i < oc->Features; i++)
                            {
                                classID = objective->GetFeatureID(i);

                                if (Falcon4ClassTable[classID].vuClassData.classInfo_[VU_TYPE] == origtype)
                                    if (objective->GetFeatureStatus(i) == 3)
                                    {
                                        vc->active = TRUE;
                                        break;
                                    }
                            }
                        }
                    }

                    break;
                }

                case vt_degrade:
                {
                    objective = (Objective) FindEntity(vc->id);

                    if (objective and objective->IsObjective())
                    {
                        //MonoPrint ("%08x = %d ", objective, objective->GetObjectiveStatus ());

                        if (objective->GetObjectiveStatus() <= 100 - vc->tolerance * 10)
                        {
                            //MonoPrint ("TRUE\n");
                            vc->active = TRUE;
                        }

                        //else
                        //{
                        // //MonoPrint ("FALSE\n");
                        // vc->active = FALSE;
                        //}
                    }

                    break;
                }

                case vt_attrit:
                {
                    unit = (Unit) FindEntity(vc->id);

                    if (unit)
                    {
                        if ( not unit->IsUnit())
                            break;

                        per = 10 * unit->GetTotalVehicles() / unit->GetFullstrengthVehicles();

                        // MonoPrint ("%08x = %d:%d %d%% ", unit, unit->GetFullstrengthVehicles (), unit->GetTotalVehicles (), per);

                        if (per <= (10 - vc->tolerance))
                        {
                            //MonoPrint ("TRUE\n");
                            vc->active = TRUE;
                        }
                        else
                        {
                            //MonoPrint ("FALSE\n");
                            vc->active = FALSE;
                        }
                    }
                    else
                    {
                        // MonoPrint ("Unit Destroyed %08x = %d\n", unit);
                        vc->active = TRUE;
                    }

                    break;
                }

                case vt_intercept:
                {
                    unit = (Unit) FindEntity(vc->id);

                    if (unit)
                    {
                        if ( not unit->IsUnit())
                            break;

                        if ( not vc->max_vehicles)
                        {
                            vc->max_vehicles = unit->GetTotalVehicles();
                        }
                    }

                    if ((unit) and (vc->max_vehicles))
                    {
                        // MonoPrint ("%08x = %d:%d ", unit, vc->number, unit->GetTotalVehicles ());

                        if (vc->max_vehicles - unit->GetTotalVehicles() >= vc->tolerance)
                        {
                            //MonoPrint ("TRUE\n");
                            vc->active = TRUE;
                        }
                        else
                        {
                            // MonoPrint ("FALSE\n");
                            vc->active = FALSE;
                        }
                    }
                    else
                    {
                        // MonoPrint ("Unit Destroyed %08x = %d\n", unit);
                        vc->active = TRUE;
                    }

                    break;
                }

                //default:
                //{
                // vc->active = FALSE;
                //}
            }

            if (old_active not_eq vc->active)
            {
                update_team_victory_window();
                //MonoPrint ("Victory Condition %d is now %s\n", count, (vc->active?"TRUE":"FALSE"));
                changed = 1;

                // Send message to remote players
                if (TheCampaign.IsMaster() and gCommsMgr->Online())
                {
                    vcdone = new UISendMsg(FalconNullId, FalconLocalGame);
                    vcdone->dataBlock.from = FalconLocalSessionId;
                    vcdone->dataBlock.msgType = UISendMsg::VC_Update;
                    vcdone->dataBlock.number = vc->get_number();
                    vcdone->dataBlock.value = vc->active;

                    FalconSendMessage(vcdone, TRUE);
                }
            }
        }

        count ++;
        vc = vc->succ;
    }

    victory_condition::leave_critical_section();

    if (changed)
        gRefreshScoresList = 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if 0 // NOT supported anymore -PJW
void tactical_mission::evaluate_parameters(void *arg_wp, double x, double y, double z, double s)
{
    victory_condition
    *vc;

    int
    old_active,
    num,
    count;

    float
    dp,
    dt,
    dx,
    dy,
    dv,
    dz;

    static float tos_table[] =
    {
        0,
        5,
        10,
        20,
        60,
        120
    },
    airspeed_table[] =
    {
        0,
        5,
        10,
        20,
        50,
        100
    },
    altitude_table[] =
    {
        50,
        100,
        200,
        500,
        1000
    },
    position_table[] =
    {
        0,
        500,
        1000,
        2000,
        5000,
        10000
    };

    WayPointClass
    *wp,
    *first_wp;

    wp = (WayPointClass *) arg_wp;

    while ((wp) and (wp->GetPrevWP()))
    {
        wp = wp->GetPrevWP();
    }

    first_wp = wp;

    victory_condition::enter_critical_section();

    vc = conditions;

    count = 1;

    while (vc)
    {
        old_active = vc->active;

        if ((vc->type == vt_tos) or (vc->type == vt_airspeed) or (vc->type == vt_altitude) or (vc->type == vt_position))
        {
            wp = first_wp;

            num = vc->data.steerpoint;

            while ((wp) and (num > 1))
            {
                wp = wp->GetNextWP();

                num --;
            }

            wp->GetLocation(&dx, &dy, &dz);

            dx = dx - x;
            dy = dy - y;
            dz = dz - z;

            dp = sqrt(dx * dx + dy * dy);

        }

        switch (vc->type)
        {
            case vt_tos:
            {
                if ((fabs(dp) < 10000) and (fabs(dz) < 1000))
                {
                    dt = ((float)wp->GetWPArrivalTime() - SimLibElapsedTime) / SEC_TO_MSEC;

                    //MonoPrint ("%d TOS %f ", count, dt);

                    if ((vc->tolerance >= 1) and (vc->tolerance <= 5) and (fabs(dt) < tos_table[vc->tolerance]))
                    {
                        //MonoPrint ("TRUE\n");
                        vc->active = TRUE;
                    }
                    else
                    {
                        //MonoPrint ("FALSE\n");
                    }
                }

                break;
            }

            case vt_airspeed:
            {
                if ((fabs(dp) < 10000) and (fabs(dz) < 1000))
                {
                    dv = s - wp->GetWPSpeed();

                    //MonoPrint ("%d AirSpeed %f ", count, dv);

                    if ((vc->tolerance >= 1) and (vc->tolerance <= 5) and (fabs(dt) < airspeed_table[vc->tolerance]))
                    {
                        //MonoPrint ("TRUE\n");
                        vc->active = TRUE;
                    }
                    else
                    {
                        //MonoPrint ("FALSE\n");
                    }
                }

                break;
            }

            case vt_altitude:
            {
                if ((fabs(dp) < 10000) and (fabs(dz) < 1000))
                {
                    //MonoPrint ("%d Alititude %f ", count, dz);

                    if ((vc->tolerance >= 1) and (vc->tolerance <= 5) and (abs(dz) < altitude_table[vc->tolerance]))
                    {
                        //MonoPrint ("TRUE\n");
                        vc->active = TRUE;
                    }
                    else
                    {
                        //MonoPrint ("FALSE\n");
                    }
                }

                break;
            }

            case vt_position:
            {
                if ((fabs(dp) < 10000) and (fabs(dz) < 1000))
                {
                    //MonoPrint ("%d Position %f ", count, dp);

                    if ((vc->tolerance >= 1) and (vc->tolerance <= 5) and (fabs(dp) < position_table[vc->tolerance]))
                    {
                        //MonoPrint ("TRUE\n");
                        vc->active = TRUE;
                    }
                    else
                    {
                        //MonoPrint ("FALSE\n");
                        vc->active = FALSE;
                    }
                }

                break;
            }
        }

        if (old_active not_eq vc->active)
        {
            //MonoPrint ("Victory Condition %d is now %s\n", count, (vc->active?"TRUE":"FALSE"));
        }

        count ++;
        vc = vc->succ;
    }

    victory_condition::leave_critical_section();
}

#endif
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_mission::calculate_victory_points(void)
{
    victory_condition::enter_critical_section();

    victory_condition
    *vc;

    int
    pts[8] =
    {
        0
    };

    vc = conditions;

    while (vc)
    {
        if (vc->active)
        {
            pts[vc->team] += vc->points;
        }

        vc = vc->succ;
    }

    memcpy(team_pts, pts, sizeof(int) * 8);

    victory_condition::leave_critical_section();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int tactical_mission::determine_victor(void)
{
    int
    i;

    UISendMsg
    *vcdone;

    if (get_game_over())
        return(TRUE);

    if (TheCampaign.GetCampaignTime() >= TheCampaign.GetTETimeLimitTime())
    {
        set_game_over(1);

        if (TheCampaign.IsMaster() and gCommsMgr->Online())
        {
            vcdone = new UISendMsg(FalconNullId, FalconLocalGame);
            vcdone->dataBlock.from = FalconLocalSessionId;
            vcdone->dataBlock.msgType = UISendMsg::VC_GameOver;

            FalconSendMessage(vcdone, TRUE);
        }

        return(TRUE);
    }

    if (points_required <= 0)
        return(FALSE);

    for (i = 0; i < 8; i++)
    {
        if (team_pts[i] >= points_required)
        {
            if (TheCampaign.IsMaster() and gCommsMgr->Online())
            {
                vcdone = new UISendMsg(FalconNullId, FalconLocalGame);
                vcdone->dataBlock.from = FalconLocalSessionId;
                vcdone->dataBlock.msgType = UISendMsg::VC_GameOver;

                FalconSendMessage(vcdone, TRUE);
            }

            set_game_over(1);
            return TRUE;
        }
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int determine_tactical_rating(void)
{
    if (current_tactical_mission)
    {
        return current_tactical_mission->determine_rating();
    }

    //I like to succeed :) BTW, we should never get here
    return mr_success;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int tactical_mission::determine_rating(void)
{
    int player_pts = 0;
    int best_opp = 0;


    evaluate_victory_conditions();
    calculate_victory_points();

    player_pts = team_pts[team];

    for (int i = 0; i < 8; i++)
    {
        if ((team_pts[i] > best_opp) and (i not_eq team))
            best_opp = team_pts[i];
    }

    if (player_pts >= points_required)
    {
        if (best_opp >= points_required)
        {
            if (player_pts > best_opp)
                return mr_marginal_victory;
            else if (player_pts < best_opp)
                return mr_marginal_defeat;
            else
                return mr_tie;
        }
        else
        {
            if (TheCampaign.TE_type not_eq tt_engagement)
                return mr_success;
            else
                return mr_decisive_victory;
        }
    }
    else
    {
        if (best_opp > points_required)
        {
            if (TheCampaign.TE_type not_eq tt_engagement)
                return mr_failure;
            else
                return mr_crushing_defeat;
        }
        else
            return mr_stalemate;
    }

    //I like to succeed :) BTW, we should never get here
    return mr_success;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
