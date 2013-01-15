#ifndef _SOUNDFX_H
#define _SOUNDFX_H

// WARNING WARNING
// THESE MUST BE IN THE SAME ORDER AS THE ENUM IN SoundFX.cpp

enum SFX_TYPES {
    SFX_NOTHING = 0 ,
	SFX_BIGGUN1, 
	SFX_BIGGUN2, 
	SFX_BOMBDROP, 
	SFX_ENGHELI, 
	SFX_EJECT, 
	SFX_GROWL, 
	SFX_GROWLOCK, 
	SFX_GRNDHIT1, 
	SFX_GRNDHIT2, 
	SFX_MISSILE1, //10
	SFX_MISSILE2, 
	SFX_MISSILE3, 
	SFX_DIRTDART, 
	SFX_H2ODART, 
	SFX_LAWNDART, 
	SFX_PROP, 
	SFX_TANK, 
	SFX_TRUCK1, 
	SFX_TRUCK2, 
	SFX_RICOCHET1, //20
	SFX_RICOCHET2, 
	SFX_RICOCHET3, 
	SFX_RICOCHET4, 
	SFX_RICOCHET5, 
	SFX_VULSTART, 
	SFX_VULLOOP, 
	SFX_VULEND, 
	SFX_BOOMG1, 
	SFX_BOOMG2, 
	SFX_BOOMG3, //30
	SFX_BOOMG4, 
	SFX_BOOMG5, 
	SFX_ENGINEA, 
	SFX_ENGINEB, 
	SFX_F16EXT, 
	SFX_F16INT, 
	SFX_Mig21_Radar, 
	SFX_GEARDN, 
	SFX_BURNERE, 
	SFX_BURNERI, //40
	SFX_Mig23_radar, 
	SFX_TOUCHDOWN, 
	SFX_BOOMA1, 
	SFX_BOOMA2, 
	SFX_BOOMA3, 
	SFX_BOOMA4, 
	SFX_BOOMA5, 
	SFX_FLAPS, 
	SFX_MCGUN, 
	SFX_BB_JAMMER, //50
	SFX_VULSTRTE, 
	SFX_Mig25_Radar, 
	SFX_Mig31_Radar, 
	SFX_IMPACTA1, 
	SFX_IMPACTA2, 
	SFX_IMPACTA3, 
	SFX_IMPACTA4, 
	SFX_IMPACTA5, 
	SFX_TWS_LOCK, 
	SFX_TWS_LAUNCH, // 60
	SFX_TWS_LAUNCH_IR, 
	SFX_TWS_SEARCH, 
	SFX_BB_WARNING, 
	SFX_BB_CAUTION, 
	SFX_BB_ALTITUDE, 
	SFX_BB_BINGO, 
	SFX_BB_LOCK, 
	SFX_BB_PULLUP, 
	SFX_BB_DATA, 
	SFX_BB_IFF, //70
	SFX_VULLOOPE, 
	SFX_VULENDE, 
	SFX_A50_Radar, 
	SFX_Chaparal_Radar, 
	SFX_F5_Radar, 
	SFX_F22_Radar, 
	SFX_2S6_Radar, 
	SFX_ADATS_Radar, 
	SFX_AH66_Radar, 
	SFX_AV8B_Radar, //80
	SFX_E2C_Radar, 
	SFX_E3_Radar, 
	SFX_F4_Radar, 
	SFX_F14_Radar, 
	SFX_F15_Radar, 
	SFX_Hawk_Radar, 
	SFX_Hercules_Radar, 
	SFX_J5_Radar, 
	SFX_J7_Radar, 
	SFX_TWI_Launch, //90
	SFX_LAV_ADV_Radar, 
	SFX_Patriot_Radar, 
	SFX_SA2_Radar, 
	SFX_SA3_Radar, 
	SFX_SA4_Radar, 
	SFX_SA5_Radar, 
	SFX_SA6_Radar, 
	SFX_SA8_Radar, 
	SFX_SA9_Radar, 
	SFX_SA10_Radar, //100
	SFX_SA13_Radar, 
	SFX_Slotback_Radar, 
	SFX_SU15_Radar, 
	SFX_DIVELOOP, 
	SFX_FIRELOOP, 
	SFX_CHUTE, 
	SFX_SPLASH, 
	SFX_RCKTLOOP, 
	SFX_IMPACTG1, 
	SFX_IMPACTG2, //110
	SFX_IMPACTG3, 
	SFX_IMPACTG4, 
	SFX_IMPACTG5, 
	SFX_IMPACTG6, 
	SFX_CP_TOGGLLIL, 
	SFX_CP_CHNGVIEW, 
	SFX_CP_EJECTLVR, 
	SFX_CP_GEARDWN, 
	SFX_CP_GEARUP, 
	SFX_CP_ICP1, //120
	SFX_CP_ICP2, 
	SFX_CP_ICPMNTRY, 
	SFX_CP_JETTISON, 
	SFX_CP_KNOBBIG, 
	SFX_CP_KNOBLIL, 
	SFX_CP_MFD, 
	SFX_CP_MOMNTARY, 
	SFX_CP_NAVKNOB, 
	SFX_CP_TOGGLBIG, 
	SFX_CP_CHAFLARE,//130 
	SFX_AIRBRAKE, 
	SFX_BIND, 
	SFX_GEARUP, 
	SFX_JETTISON, 
	SFX_SCREAM, 
	SFX_STALL, 
	SFX_WIND, 
	SFX_FLARE, 
	SFX_BRAKEND, 
	SFX_BRAKLOOP, //140
	SFX_BRAKSTRT, 
	SFX_BRAKWIND, 
	SFX_FLAPEND, 
	SFX_FLAPLOOP, 
	SFX_FLAPSTRT, 
	SFX_GEARCEND, 
	SFX_GEARCST, 
	SFX_GEARLOOP, 
	SFX_GEAROST, 
	SFX_GEAROEND, //150
	SFX_CP_UGH, 
	SFX_BAR_LOCK, 
	SFX_FIRE_CAN, 
	SFX_FLAT_FACE, 
	SFX_LONG_TRACK, 
	SFX_LOW_BLOW, 
	SFX_MPQ54, 
	SFX_MSQ48, 
	SFX_MSQ50, 
	SFX_SPOON_REST, //160
	SFX_TPS63, 
	SFX_F16_RADAR, 
	SFX_AEGIS_RADAR, 
	SFX_ZUES_RADAR, 
	SFX_AMRAAM_RADAR, 
	SFX_PHOENIX_RADAR, 
	SFX_LOWSPDTONE, 
	SFX_TAILSCRAPE, 
	SFX_GRND_RUMBLE, 
	SFX_GRND_RUMBLE_SHORT, //170
	SFX_TAXI_THUMP, 
	SFX_TIRE_SQUEAL, 
	SFX_HIT_1, 
	SFX_HIT_2, 
	SFX_HIT_3, 
	SFX_HIT_4, 
	SFX_HIT_5, 
	SFX_GROUND_CRUNCH, 
	SFX_MULTI_PROP, 
	//MI added for EWS stuff
	//JPO added for ships
	SFX_BB_COUNTER, // 180
	SFX_SHIP,
	//MI Uncaged & locked AIM-9
	SFX_NO_CAGE,
	//MI EWS
	SFX_BB_CHAFLARE,
	SFX_BB_CHFLLOW,
	SFX_BB_CHFLOUT,
	// Marco AIM-9 'Environment' sounds
	SFX_AIM9_ENVIRO_GND,
	SFX_AIM9_ENVIRO_SKY,
	SFX_THUNDER,
	SFX_RAININT,
	SFX_RAINEXT,//190
	//MI
	SFX_OVERGSPEED1,
	SFX_OVERGSPEED2,
	SFX_HOOKEND, // JB carrier
	SFX_HOOKLOOP, // JB carrier
	SFX_HOOKSTRT, // JB carrier
	SFX_FLATLIB,//ME123 NEW RWR SOUNDS
	SFX_TOMBSTONE,
	SFX_CLAMSHELL,
	SFX_BIGBIRD,
	SFX_SQUATEYE,//200
	SFX_TALLKING,
	SFX_SIDENET,
	SFX_BACKNET,
	SFX_KNIFEREST,
	SFX_GRILLPAN,
	SFX_BILLBOARD,
	SFX_HIGHSCREEN,
	SFX_ODDPAIR,
	SFX_PATHAND,
	SFX_THINSKIN,//210
	SFX_TINSHIELD,
	SFX_GINSLING,
	SFX_FLAPWHEEL,
	SFX_DOGEAR,
	SFX_BIGBACK,
	SFX_BACKTRAP,
	SFX_MIKECLICK,//me123
	SFX_DRAGCHUTE,
	SFX_MISSILE4 = 279, // RV - I-Hawk - Added a 4th missile launch sound
	// skip 
	SFX_BB_ALLWORDS = 270, // MLR
	SFX_SONIC_BOOM = 282,
	SFX_LAST // must be the last entry
}; 


typedef struct SfxDef
{
   char fileName[64];
   int   offset;
   int   length;
   int   handle;
   float maxDistSq;
   float maxVol;
   float minVol;
   float distSq;         // MLR 12/2/2003 - unused
   int   override;
   unsigned int lastFrameUpdated; // MLR 12/2/2003 - unused 
   unsigned int flags;
   float pitchScale;     // MLR 12/2/2003 - unused
   float curPitchScale;  // MLR 12/2/2003 - unused
   int   soundGroup;
   //int   majorSymbol, minorSymbol;
   int   LinkedSoundID, Unused;
   float min3ddist;
   float coneInsideAngle, 
	     coneOutsideAngle,
		 coneOutsideVol;
} SFX_DEF_ENTRY;

extern SFX_DEF_ENTRY BuiltinSFX[];
extern const int BuiltinNSFX;
extern SFX_DEF_ENTRY *SFX_DEF;
extern int NumSFX;
extern const char *FALCONSNDTABLE;

enum SFX_FLAGS {
    SFX_POSITIONAL   = 0x00000001, // obsolete
    SFX_POS_LOOPED   = 0x00000002,
    SFX_POS_EXTERN   = 0x00000004, // external sound that is always attenuated, also attenuated further when heard from inside closed pit
    SFX_FLAGS_VMS	 = 0x00000008, // part of the betty system.
    SFX_FLAGS_3D	 = 0x00000010, // 3d Sound
    SFX_FLAGS_FREQ	 = 0x00000020, // may change frequency
    SFX_FLAGS_PAN	 = 0x00000040, // obsolete // may do panning
    SFX_FLAGS_HIGH	 = 0x00000080, // high priority - assign first
    SFX_FLAGS_LOW	 = 0x00000100, // obsolete // low priority - software only maybe // MLR was same as HIGH!?
	SFX_FLAGS_REVDOP = 0x00000200, // reverses doppler effect - i think this will make the AB soune more realistic
	SFX_POS_SELF     = 0x00000400, // external sound that originates from self, can be heard at full volume inside, and externally attenuated
	SFX_POS_INSIDE   = 0x00000800, // sound can only be heard when it originates from self while inside the pit.
	SFX_FLAGS_CONE   = 0x00001000, // sound has a sound cone - not implemented yet 
	SFX_POS_EXTONLY  = 0x00002000, // external sound is ONLY played when the viewer is outside the pit, or canopy is open
	SFX_POS_EXTINT   = 0x00004000, // external sound that is ONLY played while in the pit. This, in combination with the above,
	                               // will be usefull when a sound needs to be 3d, and sound differently while in the pit.
};

#define SFX_TABLE_VRSN 0x2 // version number of the table
#endif
