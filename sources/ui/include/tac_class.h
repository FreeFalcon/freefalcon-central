//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#ifndef _VC_HEADER_
#define _VC_HEADER_

#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class tactical_mission;
class C_Base;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enum victory_type
{
	vt_unknown,
	vt_occupy,
	vt_destroy,
	vt_attrit,
	vt_intercept,
	vt_degrade,
	vt_kills,
	vt_frags,
	vt_deaths,
	vt_landing,
	vt_tos,
	vt_airspeed,
	vt_altitude,
	vt_position
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enum mission_rating
{
	mr_success,
	mr_decisive_victory,
	mr_marginal_victory,
	mr_tie,
	mr_stalemate,
	mr_marginal_defeat,
	mr_crushing_defeat,
	mr_failure,
};
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

enum kill_category
{
	kc_unknown,
	kc_aircraft,
	kc_ground_vehicles,
	kc_air_defences,
	kc_static_targets,
	kc_any
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

enum condition_parameters
{
	cp_unknown,
	cp_tos,
	cp_airspeed,
	cp_altitude,
	cp_position
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class victory_condition
{

private:

	friend tactical_mission;

	tactical_mission	*mission;

	int			active;

	victory_condition	*pred;
	victory_condition	*succ;

	int			team; // Team # of team who can benefit from this VC

	victory_type		type;

	VU_ID	id;

	int				feature_id; // this is the Feature ID (if vu_id is an Objective)

	int				tolerance; // this is the % for degrade/attrit or # for intercept (if vu_id is flight or unit)

	int				points; // This is the # points you get for doing the VC

	int				number; // this is the VC ID

	int				max_vehicles;	// number of aircraft in intercept

public:

	C_Base *control;

public:

	victory_condition (tactical_mission *);
	~victory_condition ();

	void set_team (int);
	int get_team (void);

	void set_type (victory_type);
	victory_type get_type (void);

	void set_vu_id (VU_ID);
	VU_ID get_vu_id (void);
	
	void set_sub_objective (int);
	int get_sub_objective (void);

	void set_tolerance (int);
	int get_tolerance (void);

	void set_points (int);
	int get_points (void);

	void set_number (int);
	int get_number (void);

	void set_active (int);
	int get_active (void);

	void test (void);

	static void enter_critical_section (void);
	static void leave_critical_section (void);
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

enum tactical_mode
{
	tm_unknown,
	tm_training,
	tm_load,
	tm_join
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

enum tactical_type
{
	tt_unknown,
	tt_engagement,
	tt_single,
	tt_training
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enum victory_condition_filter
{
	vcf_unknown,
	vcf_all,
	vcf_team,
	vcf_all_achieved,
	vcf_all_remaining,
	vcf_team_achieved,
	vcf_team_remaining
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enum tac_flags
{
	tf_hide_enemy		= 0x01,
	tf_lock_ato			= 0x02,
	tf_lock_oob			= 0x04,
	tf_start_paused		= 0x08
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class tactical_mission
{

private:

	friend victory_condition;

	char				*filename;

	int					is_new;
	int					is_online;

	int					number_teams;
	char				*team_name[8];
	char				team_flag[8];
	int					number_aircraft[8];
	int					number_players[8];
	int					number_f16s[8];

	victory_condition	*conditions;
	victory_condition	*current_vc;

	int					team;
	int					team_pts[8];

	victory_condition_filter filter;

	int					points_required;

	int					game_over;
//	static tactical_mode		search_mode;
//	static tactical_mission		*search_mission;

	char *read_te_file (char *filename, int *size);
	void process_load (char *data, int size, int full_load);

public:

	tactical_mission (char *filename);
	tactical_mission (void);		// Online
	~tactical_mission (void);

	void load (void);
	void preload (void);
	void new_setup (void);
	void info_load (char *filename);
	void revert (void);

	void save_data (char *filename);
	void save (char *filename);

	void set_game_over(int val) { game_over=val; }
	int get_game_over() { return(game_over); }

	tactical_type get_type (void);
	void set_type (tactical_type type);

	char *get_title (void);

	int get_number_of_teams (void);
	int get_number_of_aircraft (int team);
	int get_number_of_players (int team);
	int get_number_of_f16s (int team);

	int get_team_points (int theteam)			{return team_pts[theteam];}

	char *get_team_name (int team);
	int get_team_colour (int team);
	int get_team (void)						{return team;}
	int get_team_flag (int team);

	int hide_enemy_on(void);
	int lock_ato_on(void);
	int lock_oob_on(void);
	int start_paused_on(void);

	int is_flag_on(long value);

	// Moved the routines themselves into tac_class.cpp just incase any other insestuos(sp?) headers are necessary
	void set_flag(long value);
	void clear_flag(long value);

	void set_team_name (int team, char *name);
	void set_team_colour (int team, int colour);
	void set_team (int newteam)				{team = newteam;}
	void set_team_flag (int team, int flag);

	void setup_victory_condition (char *data);

	void evaluate_victory_conditions (void);
//	NOT supported... some variables have been removed or changed
//	void evaluate_parameters (void *wp, double x, double y, double z, double s);

	void calculate_victory_points(void);
	int  determine_victor(void);
	int determine_rating(void);

	void set_victory_condition_filter (victory_condition_filter);
	void set_victory_condition_team_filter (int team);

	victory_condition *get_first_victory_condition (void);
	victory_condition *get_next_victory_condition (void);

	victory_condition *get_first_unfiltered_victory_condition (void);
	victory_condition *get_next_unfiltered_victory_condition (void);

	void set_points_required (int value);
	int get_points_required (void);

};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#endif