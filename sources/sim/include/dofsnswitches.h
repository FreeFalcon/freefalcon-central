#ifndef _DOFS_N_SWITCHES_H_
#define _DOFS_N_SWITCHES_H_

enum Switches{
	COMP_AB				= 0,

	/* MLR 2/22/2004 - Do not use these ID's in the code anymore, use the arrays at the bottom of this file 
	COMP_NOS_GEAR_SW	= 1,
	COMP_LT_GEAR_SW		= 2,
	COMP_RT_GEAR_SW		= 3,
	*/

	/* MLR 2/22/2004 - Do not use these ID's in the code anymore, use the arrays at the bottom of this file */
	COMP_GEAR_SW_1		= 1,
	COMP_GEAR_SW_2		= 2,
	COMP_GEAR_SW_3		= 3,

	COMP_NOS_GEAR_ROD	= 4,
	COMP_CANOPY			= 5,
	COMP_WING_VAPOR		= 6,
	COMP_TAIL_STROBE	= 7,
	COMP_NAV_LIGHTS		= 8,
	COMP_LAND_LIGHTS	= 9,
	COMP_EXH_NOZZLE		= 10,
	COMP_TIRN_POD		= 11,
	COMP_HTS_POD		= 12,
	COMP_REFUEL_DR		= 13,

	/* MLR 2/22/2004 - Do not use these ID's in the code anymore, use the arrays at the bottom of this file 
	COMP_NOS_GEAR_DR_SW	= 14,
	COMP_LT_GEAR_DR_SW	= 15,
	COMP_RT_GEAR_DR_SW	= 16,
	COMP_NOS_GEAR_HOLE	= 17,
	COMP_LT_GEAR_HOLE	= 18,
	COMP_RT_GEAR_HOLE	= 19,
	*/

	/* MLR 2/22/2004 - Do not use these ID's in the code anymore, use the arrays at the bottom of this file */
	COMP_GEAR_DR_SW_1	= 14,
	COMP_GEAR_DR_SW_2	= 15,
	COMP_GEAR_DR_SW_3	= 16,
	COMP_GEAR_HOLE_1	= 17,
	COMP_GEAR_HOLE_2	= 18,
	COMP_GEAR_HOLE_3	= 19,

	/* MLR 2/22/2004 - Do not use these ID's in the code anymore, use the arrays at the bottom of this file 
	COMP_BROKEN_NOS_GEAR_SW= 20,
	COMP_BROKEN_LT_GEAR_SW = 21,
	COMP_BROKEN_RT_GEAR_SW = 22,
	*/

	/* MLR 2/22/2004 - Do not use these ID's in the code anymore, use the arrays at the bottom of this file */
	COMP_GEAR_BROKEN_SW_1 = 20,
	COMP_GEAR_BROKEN_SW_2 = 21,
	COMP_GEAR_BROKEN_SW_3 = 22,

	COMP_HOOK		= 23,
	COMP_DRAGCHUTE		= 24, // landing drag chute

	// MLR 2003-10-05 
	COMP_PIT_AB				= 25, // these are reserved for the 3d pits
	COMP_PIT_NOS_GEAR_SW	= 26, // since the 1st 4 switches & #7 are used by pit specific data,
	COMP_PIT_LT_GEAR_SW		= 27, // these are set to be equal to dofs 0 - 3
	COMP_PIT_RT_GEAR_SW		= 28, //
	COMP_PIT_TAIL_STROBE    = 29,
	COMP_AB2				= 30,
	COMP_EXH_NOZZLE2		= 31,

	/* Do not use these ID's in the code anymore, use the arrays at the bottom of this file */
	COMP_GEAR_SW_4	= 32,
	COMP_GEAR_SW_5	= 33,
	COMP_GEAR_SW_6	= 34,
	COMP_GEAR_SW_7	= 35,
	COMP_GEAR_SW_8	= 36,

	/* Do not use these ID's in the code anymore, use the arrays at the bottom of this file */
	COMP_GEAR_DR_SW_4	= 37,
	COMP_GEAR_DR_SW_5	= 38,
	COMP_GEAR_DR_SW_6	= 39,
	COMP_GEAR_DR_SW_7	= 40,
	COMP_GEAR_DR_SW_8	= 41,

	/* Do not use these ID's in the code anymore, use the arrays at the bottom of this file */
	COMP_GEAR_HOLE_4	= 42,
	COMP_GEAR_HOLE_5	= 43,
	COMP_GEAR_HOLE_6	= 44,
	COMP_GEAR_HOLE_7	= 45,
	COMP_GEAR_HOLE_8	= 46,
	COMP_UNUSED			= 47, //sfr: was light flash


	/* Do not use these ID's in the code anymore, use the arrays at the bottom of this file */
	COMP_GEAR_BROKEN_SW_4 = 48,
	COMP_GEAR_BROKEN_SW_5 = 49,
	COMP_GEAR_BROKEN_SW_6 = 50,
	COMP_GEAR_BROKEN_SW_7 = 51,
	COMP_GEAR_BROKEN_SW_8 = 52,

	COMP_WEAPON_BAY_0_SW = 53,
	COMP_WEAPON_BAY_1_SW = 54,
	COMP_WEAPON_BAY_2_SW = 55,
	COMP_WEAPON_BAY_3_SW = 56,
	COMP_WEAPON_BAY_4_SW = 57,

	// 3D cockpit Switch ID's move here from below (problem with ID's > 255)
	COMP_3DPIT_TAIL_STROBE_SW = 80, // Tail Strobe switch 2 states 
	COMP_3DPIT_TRIM_PITCH_SW = 81, // Trim Pitch indicator - 8 states 
	COMP_3DPIT_TRIM_YAW_SW = 82, // Trim Yaw indicator - 8 states 
	COMP_3DPIT_TRIM_ROLL_SW = 83, // Trim Roll indicator - 8 states 
	// Indicator Lights (States: Off = 0, On = 1)
	COMP_3DPIT_TFR_STBY = 84, // Light - TFR STBY 
	COMP_3DPIT_ECM_PWR = 85, // Light - ECM Power 
	COMP_3DPIT_ECM_FAIL = 86, // Light - ECM Fail 
	COMP_3DPIT_EPU_ON = 87, // Light - EPU run light 
	COMP_3DPIT_JFS_ON = 88, // Light - JFS run light 
	COMP_3DPIT_EPU_HYD = 89, // Light - EPU Hydrazine 
	COMP_3DPIT_EPU_AIR = 90, // Light - EPU Air 
	COMP_3DPIT_PWR_FLCSPGM = 91, // Light - Main Power - FLCSPGM 
	COMP_3DPIT_PWR_MAINGEN = 92, // Light - Main Power - MAINGEN 
	COMP_3DPIT_PWR_STBYGEN = 93, // Light - Main Power - STBYGEN 
	COMP_3DPIT_PWR_EPUGEN = 94, // Light - Main Power - EPUGEN 
	COMP_3DPIT_PWR_EPUPMG = 95, // Light - Main Power - EPUPMG 
	COMP_3DPIT_PWR_TOFLCS = 96, // Light - Main Power - TOFLCS 
	COMP_3DPIT_PWR_FLCSRLY = 97, // Light - Main Power - FLCSRLY 
	COMP_3DPIT_PWR_BATFAIL = 98, // Light - Main Power - BATFAIL 
	COMP_3DPIT_AVTR_ON = 99, // Light - AVTR run light 


	//ATARIBABY start new 3d cockpit switches
	COMP_3DPIT_BACKUP_ADI_OFFMARK = 100, //Backup OFF flag - only visible on ramp-start with cold jet
	COMP_3DPIT_ARNWS_LIGHT = 101, //AR/NWS console NWS light
	COMP_3DPIT_ARRDY_LIGHT = 102, //AR/NWS console RDY light
	COMP_3DPIT_ARDISC_LIGHT = 103, //AR/NWS console DISC light
	COMP_3DPIT_AOAON_LIGHT = 104, //AOA indexer console ON light
	COMP_3DPIT_AOABELOW_LIGHT = 105, //AOA indexer console below light
	COMP_3DPIT_AOAABOVE_LIGHT = 106, //AOA indexer console above light
	COMP_3DPIT_ALTPNEU_FLAG = 107, //ALT instrumnet PNEU flag
	COMP_3DPIT_ILS_VISIBLE = 108, //Make ILS needles on main ADI hide/show
	COMP_3DPIT_EYEBROW_ENGFIRE = 109, //RIGHT EYEBROW caution light ENG FIRE
	COMP_3DPIT_EYEBROW_ENGINE = 110, //RIGHT EYEBROW caution light ENGINE
	COMP_3DPIT_EYEBROW_HYDOIL = 111, //RIGHT EYEBROW caution light HYD/OIL PRESS
	COMP_3DPIT_EYEBROW_FLCS = 112, //RIGHT EYEBROW caution light FLCS/DBU ON
	COMP_3DPIT_EYEBROW_TOLDG = 113, //RIGHT EYEBROW caution light TO/LDG CONFIG
	COMP_3DPIT_EYEBROW_CANOPY = 114, //RIGHT EYEBROW caution light CANOPY OXY LOW
	COMP_3DPIT_EYEBROW_TFFAIL = 115, //LEFT EYEBROW caution light TR-FAIL
	COMP_3DPIT_ADI_LOC_FLAG = 116, //MAIN ADI LOC flag
	COMP_3DPIT_ADI_GS_FLAG = 117, //MAIN ADI GS flag
	COMP_3DPIT_ADI_OFF_FLAG = 118, //MAIN ADI OFF flag
	COMP_3DPIT_ADI_AUX_FLAG = 119, //MAIN ADI AUX flag
	COMP_3DPIT_HSI_OFF_FLAG = 120, //HSI OFF flag
	COMP_3DPIT_HSI_TO_FLAG = 121, //HSI TO flag
	COMP_3DPIT_HSI_FROM_FLAG = 122, //HSI FROM flag
	COMP_3DPIT_HSI_ILSWARN_FLAG = 123, //HSI ILSWARN flag
	COMP_3DPIT_HSI_CRSWARN_FLAG = 124, //HSI CRSWARN flag
	COMP_3DPIT_AOA_OFF_FLAG = 125, //AOA OFF flag
	COMP_3DPIT_VVI_OFF_FLAG = 126, //VVI OFF flag
	COMP_3DPIT_INTERIOR_LIGHTS = 127, //Interior lights (general lighting)
	COMP_3DPIT_INSTRUMENT_LIGHTS = 128, //Instrument lights (instrument back lighting)
	//ATARIBABY end

	// FRB - 3D cockpit toggle switches and knobs (animation)
	COMP_3DPIT_ICP_DRIFTCO = 129, // ICP DriftCo switch - 2 states
	COMP_3DPIT_ICP_WARNRESET = 130, // ICP WarnReset switch - 2 state momentary
	COMP_3DPIT_ICP_DED = 131, // ICP DED Up/Down/Seq/Reset switch - 5 states
	COMP_3DPIT_ICP_NEXT = 132, // ICP Next switch - 2 states
	COMP_3DPIT_ICP_PREVIOUS = 133, // ICP Previous switch - 2 states
	COMP_3DPIT_SILENCE_HORN = 134, // Silence Horn switch - 2 states
	COMP_3DPIT_MASTER_ARM = 135, // Master Arm switch - 3 states
	COMP_3DPIT_RT_AP_SW = 136, // Right Autopilot switch - 3 states
	COMP_3DPIT_LT_AP_SW = 137, // Left Autopilot switch - 3 states
	COMP_3DPIT_STORES_CAT = 138, // Stores Config switch - 2 states
	COMP_3DPIT_HOOK = 139, // Hook switch - 2 states
	COMP_3DPIT_ALT_GEAR = 140, // Alternate Gear switch - 2 states
	COMP_3DPIT_PARK_BRAKE = 141, // Parking Brake switch - 2 states
	COMP_3DPIT_RF_QUIET = 142, // RF Quiet control switch - 3 states
	COMP_3DPIT_IFF_QUERY = 143, // IFF Ident query switch - 2 states
	COMP_3DPIT_LASER_ARM = 144, // Laser Arm switch - 2 states
	COMP_3DPIT_ICP_SYM_WHEEL = 145, // ICP Sym wheel (color) switch - 10 states (?)
	COMP_3DPIT_ICP_BRT_WHEEL = 146, // ICP Bright wheel (brightness/contrast) switch - 10 states (?)
	COMP_3DPIT_HUD_PWR = 147, // HUD power switch - 2 states
	COMP_3DPIT_RWR_PWR = 148, // RWR power switch - 2 states
	COMP_3DPIT_RWR_SEARCH = 149, // RWR Search switch - 2 states
	COMP_3DPIT_RWR_GND_PRI = 150, // RWR Ground Priority switch - 2 states
	COMP_3DPIT_RWR_UNKS = 151, // RWR Unknowns switch - 2 states
	COMP_3DPIT_RWR_TGT_SEP = 152, // RWR Target Separation switch - 2 states
	COMP_3DPIT_RWR_PRIORITY = 153, // RWR Priority switch - 2 states
	COMP_3DPIT_RWR_HNDOFF = 154, // RWR Hand Off switch - 2 states
	COMP_3DPIT_RWR_NAVAL = 155, // RWR Naval switch - 2 states
	COMP_3DPIT_HSI_MODE = 156, // HSI Mode switch - 4 states
	COMP_3DPIT_HSI_HEADING = 157, // HSI Heading knob switch - 10 states (?)
	COMP_3DPIT_HSI_COURSE = 158, // HSI Course knob switch - 10 states (?)
	COMP_3DPIT_FUEL_QTY = 159, // Fuel Quantity Sel switch - 6 states
	COMP_3DPIT_FUEL_EXT_TRANS = 160, // External Fuel Transfer switch - 2 states
	COMP_3DPIT_EWS_RWR_PWR = 161, // EWS RWR switch - 2 states
	COMP_3DPIT_EWS_JMR_PWR = 162, // EWS Jammer switch - 2 states
	COMP_3DPIT_EWS_CHAFF_PWR = 163, // EWS Chaff switch - 2 states
	COMP_3DPIT_EWS_FLARE_PWR = 164, // EWS Flare switch - 2 states
	COMP_3DPIT_EWS_MODE = 165, // EWS Mode switch - 5 states
	COMP_3DPIT_EWS_PROG = 166, // EWS Program selection switch - 4 states
	COMP_3DPIT_IDLE_DETENT = 167, // Throttle Idle Detent switch - 2 states
	COMP_3DPIT_UHF_CH = 168, // UHF channel switch - 8 states
	COMP_3DPIT_COMM1_VOL = 169, // Comm 1 volume control switch - 10 states
	COMP_3DPIT_COMM2_VOL = 170, // Comm 2 volume control switch - 10 states
	COMP_3DPIT_MISSILE_VOL = 171, // Missile volume control switch - 10 states
	COMP_3DPIT_THREAT_VOL = 172, // Threat volume control switch - 10 states
	COMP_3DPIT_AVTR_SW = 173, // AVTR switch - 3 states
	COMP_3DPIT_MPO = 174, // MPO switch - 2 states
	COMP_3DPIT_CANOPY = 175, // Canopy switch - 2 states
	COMP_3DPIT_JSF_START = 176, // JSF start switch - 2 states
	COMP_3DPIT_MAIN_PWR = 177, // Main power switch - 3 states
	COMP_3DPIT_EPU = 178, // EPU switch - 3 states
	COMP_3DPIT_IFF_PWR = 179, //  switch - 2 states
	COMP_3DPIT_AUX_COMM_MSTR = 180, // AUX Comm Master (TACAN) switch - 5 states
	COMP_3DPIT_AUX_COMM_SRC = 181, // Aux Comm Source switch - 2 states
	COMP_3DPIT_TACAN_BAND = 182, // TACAN Channel Band switch - 2 states
	COMP_3DPIT_REFUEL_DOOR = 183, // Refuel door switch - 2 states
	COMP_3DPIT_REFUEL_PUMP = 184, // Refuel Pump switch - 4 states
	COMP_3DPIT_REFUEL_MSTR = 185, // Refuel On/Off switch - 2 states
	COMP_3DPIT_EXT_LITE_MSTR = 186, // External Lights Master switch - 2 states
	COMP_3DPIT_ANTI_COLL = 187, // Anti-collision Light switch - 2 states
	COMP_3DPIT_EXT_FLASH = 188, // External lights flash switch - 2 states
	COMP_3DPIT_EXT_WING = 189, // External wing light switch - 2 states
	COMP_3DPIT_EXT_FUSELAGE = 190, // External fuselage light switch - 2 states
	COMP_3DPIT_TRIM_AP = 191, // Trim AP Disc switch - 2 states
	COMP_3DPIT_LEF_FLAPS = 192, // LEF  switch - 2 states
	COMP_3DPIT_ALT_FLAPS = 193, // Alt Flaps switch - 2 states
	COMP_3DPIT_LEFT_HPT_PWR = 194, // Left hardpoints power switch - 2 states
	COMP_3DPIT_RIGHT_HPT_PWR = 195, // Right hardpoints power switch - 2 states
	COMP_3DPIT_FCR_PWR = 196, // FCR power switch - 2 states
	COMP_3DPIT_RALT_PWR = 197, // Alt Radar power switch - 3 states
	COMP_3DPIT_HUD_VAH = 198, // VAH switch - 3 states
	COMP_3DPIT_HUD_FPM_LADD = 199, // FPM Ladder switch - 3 states
	COMP_3DPIT_HUD_DED_PFL = 200, // DED/PFL switch - 3 states
	COMP_3DPIT_HUD_RETICLE = 201, // Reticle switch - 3 states
	COMP_3DPIT_HUD_VELOCITY = 202, // CAS/TAS switch - 3 states
	COMP_3DPIT_HUD_RAL_BARO = 203, // RAlt/Baro Altitude switch - 3 states
	COMP_3DPIT_HUD_DAY_NITE = 204, // Day/Night switch - 3 states
	COMP_3DPIT_INTERIOR_LITE = 205, // Cockpit interior lighting switch - 3 states
	COMP_3DPIT_INSTR_LITE = 206, // Cockpit intruments lighting switch - 3 states
	COMP_3DPIT_AIR_SOURCE = 207, // Air source switch - 4 states
	COMP_3DPIT_VMS_PWR = 208, // VMS power switch - 2 states
	COMP_3DPIT_FCC_PWR = 209, // FCC power switch - 2 states
	COMP_3DPIT_SMS_PWR = 210, // SMS power switch - 2 states
	COMP_3DPIT_MFD_PWR = 211, // MFD power switch - 2 states
	COMP_3DPIT_UFC_PWR = 212, // UFC power switch - 2 states
	COMP_3DPIT_GPS_PWR = 213, // GPS power switch - 2 states
	COMP_3DPIT_DL_PWR = 214, // DL power switch - 2 states
	COMP_3DPIT_MAP_PWR = 215, // MAP power switch - 2 states
	COMP_3DPIT_INS_MODE = 216, // INS mode switch - 4 states
	COMP_3DPIT_SPOT_LITE = 217, // Cockpit intruments lighting switch - 3 states
	COMP_3DPIT_REV_THRUSTER = 218, // Reverse thruster switch (2D CB: 253)
	COMP_3DPIT_LAND_LIGHT = 219, // Landing lights switch
	COMP_3DPIT_DRAGCHUTE = 220, // landing drag chute
	COMP_3DPIT_SEAT_ARM = 221, // Ejection Seat arm switch - 2 states
	// Lights
	COMP_3DPIT_RWR_LAUNCH = 222, // RWR missile launch light - 2 states

	COMP_3DPIT_FAULT_COL1_1 = 223, // Fault panel lights - Column 1 Row 1
	COMP_3DPIT_FAULT_COL1_2 = 224, // Fault panel lights -					row 2
	COMP_3DPIT_FAULT_COL1_3 = 225, // Fault panel lights - 					row 3
	COMP_3DPIT_FAULT_COL1_4 = 226, // Fault panel lights - 					row 4
	COMP_3DPIT_FAULT_COL1_5 = 227, // Fault panel lights - 					row 5
	COMP_3DPIT_FAULT_COL1_6 = 228, // Fault panel lights - 					row 6
	COMP_3DPIT_FAULT_COL1_7 = 229, // Fault panel lights - 					row 7
	COMP_3DPIT_FAULT_COL1_8 = 230, // Fault panel lights - 					row 8

	COMP_3DPIT_FAULT_COL2_1 = 231, // Fault panel lights - Column 2 Row 1
	COMP_3DPIT_FAULT_COL2_2 = 232, // Fault panel lights -					row 2
	COMP_3DPIT_FAULT_COL2_3 = 233, // Fault panel lights - 					row 3
	COMP_3DPIT_FAULT_COL2_4 = 234, // Fault panel lights - 					row 4
	COMP_3DPIT_FAULT_COL2_5 = 235, // Fault panel lights - 					row 5
	COMP_3DPIT_FAULT_COL2_6 = 236, // Fault panel lights - 					row 6
	COMP_3DPIT_FAULT_COL2_7 = 237, // Fault panel lights - 					row 7
	COMP_3DPIT_FAULT_COL2_8 = 238, // Fault panel lights - 					row 8

	COMP_3DPIT_FAULT_COL3_1 = 239, // Fault panel lights - Column 3 Row 1
	COMP_3DPIT_FAULT_COL3_2 = 240, // Fault panel lights -					row 2
	COMP_3DPIT_FAULT_COL3_3 = 241, // Fault panel lights - 					row 3
	COMP_3DPIT_FAULT_COL3_4 = 242, // Fault panel lights - 					row 4
	COMP_3DPIT_FAULT_COL3_5 = 243, // Fault panel lights - 					row 5
	COMP_3DPIT_FAULT_COL3_6 = 244, // Fault panel lights - 					row 6
	COMP_3DPIT_FAULT_COL3_7 = 245, // Fault panel lights - 					row 7
	COMP_3DPIT_FAULT_COL3_8 = 246, // Fault panel lights - 					row 8

	COMP_3DPIT_FAULT_COL4_1 = 247, // Fault panel lights - Column 4 Row 1
	COMP_3DPIT_FAULT_COL4_2 = 248, // Fault panel lights -					row 2
	COMP_3DPIT_FAULT_COL4_3 = 249, // Fault panel lights - 					row 3
	COMP_3DPIT_FAULT_COL4_4 = 250, // Fault panel lights - 					row 4
	COMP_3DPIT_FAULT_COL4_5 = 251, // Fault panel lights - 					row 5
	COMP_3DPIT_FAULT_COL4_6 = 252, // Fault panel lights - 					row 6
	COMP_3DPIT_FAULT_COL4_7 = 253, // Fault panel lights - 					row 7
	COMP_3DPIT_FAULT_COL4_8 = 254, // Fault panel lights - 					row 8

	// Moved above to ID's #81+
	//COMP_3DPIT_TRIM_PITCH_SW = 255, // Trim Pitch indicator - 8 states 
	//COMP_3DPIT_TRIM_YAW_SW = 256, // Trim Yaw indicator - 8 states 
	//COMP_3DPIT_TRIM_ROLL_SW = 257, // Trim Roll indicator - 8 states 
	// end switches and knobs (animation)
	// Indicator Lights (States: Off = 0, On = 1)
	//COMP_3DPIT_TFR_STBY = 258, // Light - TFR STBY 
	//COMP_3DPIT_ECM_PWR = 259, // Light - ECM Power 
	//COMP_3DPIT_ECM_FAIL = 260, // Light - ECM Fail 
	//COMP_3DPIT_EPU_ON = 261, // Light - EPU run light 
	//COMP_3DPIT_JFS_ON = 262, // Light - JFS run light 
	//COMP_3DPIT_EPU_HYD = 263, // Light - EPU Hydrazine 
	//COMP_3DPIT_EPU_AIR = 264, // Light - EPU Air 
	//COMP_3DPIT_PWR_FLCSPGM = 265, // Light - Main Power - FLCSPGM 
	//COMP_3DPIT_PWR_MAINGEN = 266, // Light - Main Power - MAINGEN 
	//COMP_3DPIT_PWR_STBYGEN = 267, // Light - Main Power - STBYGEN 
	//COMP_3DPIT_PWR_EPUGEN = 268, // Light - Main Power - EPUGEN 
	//COMP_3DPIT_PWR_EPUPMG = 269, // Light - Main Power - EPUPMG 
	//COMP_3DPIT_PWR_TOFLCS = 270, // Light - Main Power - TOFLCS 
	//COMP_3DPIT_PWR_FLCSRLY = 271, // Light - Main Power - FLCSRLY 
	//COMP_3DPIT_PWR_BATFAIL = 272, // Light - Main Power - BATFAIL 
	//COMP_3DPIT_AVTR_ON = 273, // Light - AVTR run light 



	COMP_MAX_SWITCH		= 255,	// update!
//*************************************************

	SIMP_AB				= 0,
	SIMP_TANKERLIGHTS	= 1,
	SIMP_GEAR			= 2,
	SIMP_WING_VAPOR		= 3,
	SIMP_CANOPY			= 5,
	SIMP_DRAGCHUTE		= 6,
	SIMP_HOOK		= 7,
	SIMP_MAX_SWITCH		= 8,

	HELI_ROTORS		= 0,
	HELI_MAX_SWITCH		 = 2,

	AIRDEF_LIGHT_SWITCH = 2,
	AIRDEF_MAX_SWITCH	= 3,
};

enum DOFS{
	COMP_LT_STAB		= 0,
	COMP_RT_STAB		= 1,
	COMP_LT_FLAP		= 2, // Flapperons or ailerons
	COMP_RT_FLAP		= 3,
	COMP_RUDDER			= 4,
	COMP_NOS_GEAR_ROT	= 5,

	/* MLR 2/22/2004 - Do not use these ID's in the code anymore, use the arrays at the bottom of this file 
	COMP_NOS_GEAR_COMP	= 6,
	COMP_LT_GEAR_COMP	= 7,
	COMP_RT_GEAR_COMP	= 8,
	*/

	COMP_FREE1	= 6,
	COMP_FREE2	= 7,
	COMP_FREE3	= 8,

	COMP_LT_LEF			= 9,
	COMP_RT_LEF			= 10,			
	COMP_BROKEN_NOS_GEAR= 11,	
	COMP_BROKEN_LT_GEAR = 12,	
	COMP_BROKEN_RT_GEAR = 13,	
	COMP_NOTUSED_14		= 14,//available
	COMP_LT_AIR_BRAKE_TOP = 15,
	COMP_LT_AIR_BRAKE_BOT = 16,
	COMP_RT_AIR_BRAKE_TOP = 17,
	COMP_RT_AIR_BRAKE_BOT = 18,

	/* MLR 2/22/2004 - Do not use these ID's in the code anymore, use the arrays at the bottom of this file 
	COMP_NOS_GEAR		= 19,
	COMP_LT_GEAR		= 20,
	COMP_RT_GEAR		= 21,
	COMP_NOS_GEAR_DR	= 22,
	COMP_LT_GEAR_DR		= 23,
	COMP_RT_GEAR_DR		= 24, */


	/* MLR 2/22/2004 - Do not use these ID's in the code anymore, use the arrays at the bottom of this file */
	COMP_GEAR_1			= 19,
	COMP_GEAR_2			= 20,
	COMP_GEAR_3			= 21,
	COMP_GEAR_DR_1		= 22,
	COMP_GEAR_DR_2		= 23,
	COMP_GEAR_DR_3		= 24, 

	// 25-27 are used in some models (like F16) for landing gear bits
	// hence this gap
	// MLR-NOTE erm, where?

	// Cobra - Pilot head movement
	COMP_HEAD_LR		= 25, // left/right
	COMP_HEAD_UD		= 26, // up/down
	// WSO/RIO head movement
	COMP_HEAD2_LR		= 27, // left/right
	COMP_HEAD2_UD		= 49, // up/down

	COMP_LT_TEF		= 28, // JPO
	COMP_RT_TEF		= 29,
	COMP_CANOPY_DOF		= 30, // opening canopy
	// 31-37 earmarked for prop animation DOFs, so next named one should be 37 please.

  COMP_TAILHOOK =   37,
  COMP_ABDOF =      38, // afterburner DOF, for scaling.
  COMP_EXH_NOZ =    39, // exhuast nozzle

  COMP_PROPELLOR  = 40,
  COMP_REFUEL     = 41, // refuel probe
  COMP_LT_SPOILER1  = 42,
  COMP_RT_SPOILER1  = 43,
  COMP_LT_SPOILER2  = 44,
  COMP_RT_SPOILER2  = 45,

  COMP_SWING_WING = 46,
  COMP_THROTTLE   = 47,
  COMP_RPM        = 48,
	//COMP_HEAD2_UD		= 49, // see above

  COMP_WHEEL_1   =   50, // MLR 2003-10-04 // 2003-10-14 renamed
  COMP_WHEEL_2   =   51,
  COMP_WHEEL_3   =   52,
  COMP_WHEEL_4   =   53,
  COMP_WHEEL_5   =   54,
  COMP_WHEEL_6   =   55,
  COMP_WHEEL_7   =   56,
  COMP_WHEEL_8   =   57,
  COMP_GEAREXTENSION_1   =   58, // MLR 2003-10-04
  COMP_GEAREXTENSION_2   =   59,
  COMP_GEAREXTENSION_3   =   60,
  COMP_GEAREXTENSION_4   =   61,
  COMP_GEAREXTENSION_5   =   62,
  COMP_GEAREXTENSION_6   =   63,
  COMP_GEAREXTENSION_7   =   64, // hmm >64 dofs?
  COMP_GEAREXTENSION_8   =   65,
  COMP_ABDOF2			 =   66, // afterburner DOF, for scaling.
  COMP_EXH_NOZ2          =   67, // exhuast nozzle

	/* Do not use these ID's in the code anymore, use the arrays at the bottom of this file */
  	COMP_GEAR_4			= 68,
  	COMP_GEAR_5			= 69,
  	COMP_GEAR_6			= 70,
  	COMP_GEAR_7			= 71,
  	COMP_GEAR_8			= 72,

	/* Do not use these ID's in the code anymore, use the arrays at the bottom of this file */
	COMP_GEAR_DR_4		= 73,
	COMP_GEAR_DR_5		= 74,
	COMP_GEAR_DR_6		= 75,
	COMP_GEAR_DR_7		= 76,
	COMP_GEAR_DR_8		= 77,

	COMP_WEAPON_BAY_0  = 78,
	COMP_WEAPON_BAY_1  = 79,
	COMP_WEAPON_BAY_2  = 80,
	COMP_WEAPON_BAY_3  = 81,
	COMP_WEAPON_BAY_4  = 82,
//Cobra 10/30/04 TJL
	COMP_INTAKE_1_RAMP_1 = 83,
	COMP_INTAKE_1_RAMP_2 = 84,
	COMP_INTAKE_1_RAMP_3 = 85,

	COMP_INTAKE_2_RAMP_1 = 86,
	COMP_INTAKE_2_RAMP_2 = 87,
	COMP_INTAKE_2_RAMP_3 = 88,

  //ATARIBABY start new 3d cokpit dofs, i made enought hole i hope
  COMP_3DPIT_ADI_ROLL   = 100,  //main adi ball roll
  COMP_3DPIT_ADI_PITCH  = 101, //main adi ball pitch
  COMP_3DPIT_ASI_NEEDLE  = 102, //asi instrument needle
  COMP_3DPIT_BACKUP_ADI_ROLL   = 103, //backup adi ball roll
  COMP_3DPIT_BACKUP_ADI_PITCH  = 104, //backup adi ball pitch
  COMP_3DPIT_ALT_NEEDLE  = 105, //alt instrument needle
  COMP_3DPIT_ILSV_NEEDLE  = 106, //ILS vertical needle
  COMP_3DPIT_ILSH_NEEDLE  = 107, //ILS horizontal needle
  COMP_3DPIT_MAG_COMPASS  = 108, //backup magnetic compass
  COMP_3DPIT_ASIMACH_DIGIT1  = 109, //ASI mach digital readout left digit
  COMP_3DPIT_ASIMACH_DIGIT2  = 110, //ASI mach digital readout right digit
  COMP_3DPIT_ALT_DIGIT1  = 111, //ALT digital readout digit 1
  COMP_3DPIT_ALT_DIGIT2  = 112, //ALT digital readout digit 2
  COMP_3DPIT_ALT_DIGIT3  = 113, //ALT digital readout digit 3
  COMP_3DPIT_HSI_HDG  = 114, //HSI current heading
  COMP_3DPIT_HSI_CRS  = 115, //HSI desired course
  COMP_3DPIT_HSI_DHDG  = 116, //HSI desired heading
  COMP_3DPIT_HSI_BCN  = 117, //HSI beacon course
  COMP_3DPIT_HSI_CRSDEV  = 118, //HSI course deviation
  COMP_3DPIT_HSI_DIST_DIGIT1  = 119, //HSI distance to beacon digit 1
  COMP_3DPIT_HSI_DIST_DIGIT2  = 120, //HSI distance to beacon digit 2
  COMP_3DPIT_HSI_DIST_DIGIT3  = 121, //HSI distance to beacon digit 3
  COMP_3DPIT_HSI_CRS_DIGIT1  = 122, //HSI course beacon digit 1
  COMP_3DPIT_HSI_CRS_DIGIT2  = 123, //HSI course to beacon digit 2
  COMP_3DPIT_HSI_CRS_DIGIT3  = 124, //HSI course digit 3
  COMP_3DPIT_FUELFLOW_DIGIT1  = 125, //FUEL FLOW digit 1
  COMP_3DPIT_FUELFLOW_DIGIT2  = 126, //FUEL FLOW digit 2
  COMP_3DPIT_FUELFLOW_DIGIT3  = 127, //FUEL FLOW digit 3
  COMP_3DPIT_OIL_NEEDLE  = 128, //OIL needle
  COMP_3DPIT_NOZ_NEEDLE  = 129, //NOZ needle
  COMP_3DPIT_RPM_NEEDLE  = 130, //RPM needle
  COMP_3DPIT_FTIT_NEEDLE  = 131, //FTIT needle
  COMP_3DPIT_AOA  = 132, //AOA tape
  COMP_3DPIT_VVI  = 133, //VVI tape
  COMP_3DPIT_HYDA_NEEDLE  = 134, //HYD A PRESS needle
  COMP_3DPIT_HYDB_NEEDLE  = 135, //HYD B PRESS needle
  COMP_3DPIT_EPU_NEEDLE  = 136, //EPU needle
  COMP_3DPIT_FUEL_DIGIT1  = 137, //FUEL digit 1
  COMP_3DPIT_FUEL_DIGIT2  = 138, //FUEL digit 2
  COMP_3DPIT_FUEL_DIGIT3  = 139, //FUEL digit 3
  COMP_3DPIT_FUEL_DIGIT4  = 140, //FUEL digit 4
  COMP_3DPIT_FUEL_DIGIT5  = 141, //FUEL digit 5
  COMP_3DPIT_FUELAFT_NEEDLE  = 142, //FUEL AFT
  COMP_3DPIT_FUELFWD_NEEDLE  = 143, //FUEL FWD
  COMP_3DPIT_G_NEEDLE  = 144, //G-meter needle
  //ATARIBABY end

	// FRB - 3D cockpit knobs, lever, dial (animation)
	COMP_3DPIT_TACAN_LEFT = 145, // TACAN channel digit
	COMP_3DPIT_TACAN_CENTER = 146, // TACAN channel digit
	COMP_3DPIT_TACAN_RIGHT = 147, // TACAN channel digit
	COMP_3DPIT_TRIM_PITCH = 148, // Trim Pitch indicator dial
	COMP_3DPIT_TRIM_YAW = 149, // Trim Yaw indicator dial
	COMP_3DPIT_TRIM_ROLL = 150, // Trim Roll indicator dial
	COMP_3DPIT_CLOCK_HRS = 151, // Clock - hour hand
	COMP_3DPIT_CLOCK_MINS = 152, // Clock - minute hand
	COMP_3DPIT_CLOCK_SECS = 153, // Clock - second hand
	COMP_REVERSE_THRUSTER = 154, // Reverse thruster
  COMP_3DPIT_CHAFF_DIGIT1  = 155, // Chaff remaining count digit 1
  COMP_3DPIT_CHAFF_DIGIT2  = 156,  // Chaff remaining count digit 2
  COMP_3DPIT_CHAFF_DIGIT3  = 157, // Chaff remaining count digit 3
  COMP_3DPIT_FLARE_DIGIT1  = 158, // Flares remaining count digit 1
  COMP_3DPIT_FLARE_DIGIT2  = 159,  // Flares remaining count digit 2
  COMP_3DPIT_FLARE_DIGIT3  = 160, // Flares remaining count digit 3
  COMP_3DPIT_AOA_DIAL  = 161, //AOA tape
  COMP_3DPIT_VVI_DIAL  = 162, //VVI tape

	// end knobs, lever, dial (animation)

	COMP_MAX_DOF		= 163, // Make sure this is up to date!
//*************************************************

	// Simple model DOF's 
	SIMP_LT_STAB		= 0,
	SIMP_RT_STAB		= 1,
	// this gap used by animations (including 6 & 7 actually) JPO
	SIMP_LT_AILERON		= 6,
	SIMP_RT_AILERON		= 7,
	SIMP_RUDDER_1		= 8,
	SIMP_RUDDER_2		= 9,
	SIMP_AIR_BRAKE		= 10,// Jet Blast Deflector #1
	SIMP_SWING_WING_1	= 11,// Jet Blast Deflector #2
	SIMP_SWING_WING_2	= 12,// Jet Blast Deflector #3
	SIMP_SWING_WING_3	= 13,// Jet Blast Deflector #4
	SIMP_SWING_WING_4	= 14,
	SIMP_SWING_WING_5	= 15,
	SIMP_SWING_WING_6	= 16,
	SIMP_SWING_WING_7	= 17,
	SIMP_SWING_WING_8	= 18,
	SIMP_RT_TEF		= 19, // JPO - new stuff
	SIMP_LT_TEF		= 20,
	SIMP_RT_LEF		= 21,
	SIMP_LT_LEF		= 22,
	SIMP_CANOPY_DOF		= 23, // opening canopy

  SIMP_PROPELLOR  = 40,
  SIMP_REFUEL     = 41, // refuel probe


  SIMP_LT_SPOILER1  = 42,
  SIMP_RT_SPOILER1  = 43,
  SIMP_LT_SPOILER2  = 44,
  SIMP_RT_SPOILER2  = 45,

  SIMP_THROTTLE   = 47,
  SIMP_RPM        = 48,

	SIMP_MAX_DOF		= 49, // MAKE SURE THIS IS UP TO DATE


	HELI_MAIN_ROTOR		= 2,
	HELI_TAIL_ROTOR		= 4,
	HELI_MAX_DOF		= 6,

	AIRDEF_AZIMUTH		= 0,
	AIRDEF_ELEV		= 1,
	AIRDEF_ELEV2		= 2,
	// RV - Biker - Ground units now can rotate radar (DOF 5)
	//AIRDEF_MAX_DOF		= 3,
	AIRDEF_MAX_DOF		= 6,
};
// MLR 2/22/2004 -  these arrays will store the actual DOF/SWITCH ids, 
//                  seeing as they are not sequential after gear 3.
extern int ComplexGearDOF[];			// landing gear retraction/extension angle
extern int ComplexGearDoorDOF[];		// door angle
extern int ComplexGearSwitch[];			// gear visible switch
extern int ComplexGearDoorSwitch[];		// door visible switch
extern int ComplexGearHoleSwitch[];		// gear bay switch 
extern int ComplexGearBrokenSwitch[];



enum Vertices {
    AIRCRAFT_MAX_DVERTEX = 6,
	HELI_MAX_DVERTEX = 0,
	VECH_MAX_DVERTEX = 0,

};
#endif
