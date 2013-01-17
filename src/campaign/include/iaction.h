///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enum instant_action_unit_type
{
	ia_unknown,
	ia_an2,
	ia_an24,
	ia_c130,
	ia_il76,
	ia_y8,
	ia_a10,
	ia_f4g,
	ia_fb111,
	ia_il28,
	ia_mig27,
	ia_su25,
	ia_ah64,
	ia_ah64d,
	ia_ka50,
	ia_mi24,
	ia_a50,
	ia_e3,
	ia_b52g,
	ia_tu16,
	ia_ef111,
	ia_f14a,
	ia_f15c,
	ia_f4e,
	ia_f5e,
	ia_mig19,
	ia_mig21,
	ia_mig23,
	ia_mig25,
	ia_mig29,
	ia_su27,
	ia_f117,
	ia_f15e,
	ia_f16c,
	ia_f18a,
	ia_f18d,
	ia_md500,
	ia_oh58d,
	ia_il78,
	ia_kc10,
	ia_kc135,
	ia_tu16n,
	ia_ch47,
	ia_uh1n,
	ia_uh60l,
	ia_battalion,
	ia_chinese_t80,
	ia_chinese_t85,
	ia_chinese_t90,
	ia_chinese_sa6,
	ia_chinese_zu23,
	ia_chinese_hq,
	ia_chinese_inf,
	ia_chinese_mech,
	ia_chinese_sp,
	ia_chinese_art,
	ia_dprk_aaa,
	ia_drpk_sa2,
	ia_dprk_sa3,
	ia_dprk_sa5,
	ia_dprk_airmobile,
	ia_dprk_t55,
	ia_dprk_t62,
	ia_dprk_hq,
	ia_dprk_inf,
	ia_dprk_bmp1,
	ia_dprk_bmp2,
	ia_dprk_bm21,
	ia_dprk_sp122,
	ia_dprk_sp152,
	ia_dprk_frog,
	ia_dprk_scud,
	ia_dprk_tow_art,
	ia_rok_aaa,
	ia_rok_hawk,
	ia_rok_nike,
	ia_rok_m48,
	ia_rok_hq,
	ia_rok_inf,
	ia_rok_marine,
	ia_rok_m113,
	ia_rok_sp,
	ia_rok_m198,
	ia_soviet_sa15,
	ia_soviet_sa6,
	ia_soviet_sa8,
	ia_soviet_air,
	ia_soviet_t72,
	ia_soviet_t80,
	ia_soviet_t90,
	ia_soviet_eng,
	ia_soviet_hq,
	ia_soviet_inf,
	ia_soviet_marine,
	ia_soviet_mech,
	ia_soviet_scud,
	ia_soviet_frog7,
	ia_soviet_sp,
	ia_soviet_sup,
	ia_soviet_bm21,
	ia_soviet_bm24,
	ia_soviet_bm9a52,
	ia_soviet_art,
	ia_us_patriot,
	ia_us_hawk,
	ia_us_air,
	ia_us_m1,
	ia_us_m60,
	ia_us_cav,
	ia_us_eng,
	ia_us_hq,
	ia_us_inf,
	ia_us_lav25,
	ia_us_m2,
	ia_us_mirs,
	ia_us_m109,
	ia_us_sup
};

struct ia_data
{
	float						distance;
	float						aspect;
	float						altitude;
	instant_action_unit_type	type;
	int							size:4;
	int							side:8;
	int							kill:1;
	int							dumb:1;
	int							skill:4;
	int							guns:1;
	int							radar:1;
	int							heat:1;
	int							ground:1;
	int							num_vector:8;

	float						vector[32];
	float						v_dist[32];
	float						v_alt[32];
	float						v_kts[32];
};

class instant_action
{

protected:

	static long		start_time;
	static float	start_x;
	static float	start_y;

	static FlightClass		*player_flight;

	static int		generic_skill;
	static int		current_wave;
	static char		current_mode;
	static unsigned long	wave_time;
	static int		wave_created;

	static void		create_unit (ia_data &data);
	static void		create_flight (ia_data &data);
	static void		create_battalion (ia_data &data);

public:

	static void		create_player_flight (void);
	static void		move_player_flight (void);
	static void		create_wave (void);
	static void		check_next_wave (void);
	static void		create_more_stuff (void);

	static int		is_fighter_sweep (void);
	static int		is_moving_mud (void);

	static void		set_start_wave (int wave);
	static void		set_start_mode (char ch);

	static void		set_start_position (float x, float y);
	static void		get_start_position (float &x, float &y);

	static void		set_start_time (long t);
	static long		get_start_time (void);

	static long		get_wave_timeout (void);

	static void		set_campaign_time (void);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
