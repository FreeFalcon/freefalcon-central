//
// Pilot.h
//

#ifndef PILOT_H
#define PILOT_H

#include <stdio.h>
#include <tchar.h>

// =======================
// Pilot defines
// =======================

#define PILOT_AVAILABLE			0
#define PILOT_KIA				1
#define PILOT_MIA				2
#define PILOT_RESCUED			3
#define PILOT_IN_USE			4

#define PILOTS_PER_FLIGHT		4						// Maximum pilots/ac per flight
#define PILOTS_PER_SQUADRON		48						// Maximum pilots per squadron
#define	NUM_PILOT_VOICES		12
#define PILOT_SKILL_RANGE		5						// # of different AI models

#define NO_PILOT				255						// No pilot is assigned

// ===========================
// Pilot name index structure
// ===========================

class PilotClass
	{
	public:
		short		pilot_id;							// Index into the PilotInfoClass table
		uchar		pilot_skill_and_rating;				// LowByte: Skill, HiByte: Rating
		uchar		pilot_status;
		uchar		aa_kills;
		uchar		ag_kills;
		uchar		as_kills;
		uchar		an_kills;
		short		missions_flown;

	public:
		PilotClass();
// 2000-11-17 MODIFIED BY S.G. I NEED TO PASS THE 'airExperience'.
//		void ResetStats(void);
		void ResetStats(uchar airExperience);
// 2001-11-19 ADDED by M.N. for TE squad change pilots rating
		void SetTEPilotRating (uchar rating);
		int GetPilotSkill(void)		{ return (pilot_skill_and_rating & 0xF); }
		int GetPilotRating(void)	{ return ((uchar)((pilot_skill_and_rating & 0xF0) >> 4)); }
		void SetPilotSR(uchar skill, uchar rating)	{ pilot_skill_and_rating = (uchar)((rating << 4) | skill); }
	};

class PilotInfoClass
	{
	public:
		short		usage;								// How many times this pilot is being used
		uchar		voice_id;							// Which voice data to use
		uchar		photo_id;							// Assigned through the UI
	public:
		PilotInfoClass();
		void ResetStats (void);
		void AssignVoice(int owner); // JPO
	};

// ================
// Data
// ================

extern uchar* CallsignData;
extern PilotInfoClass* PilotInfo;

extern int NumPilots;
extern int NumCallsigns;

// ================
// functions
// ================

class FlightClass;
typedef FlightClass *Flight;

extern void NewPilotInfo (void);

extern int LoadPilotInfo (char* filename);

extern void SavePilotInfo (char* filename);

extern void DisposePilotInfo (void);

extern int GetAvailablePilot (int first, int last, int owner);

extern void GetPilotName (int id, _TCHAR* name, int size);

extern void GetCallsignID (uchar* id, uchar* num, int range);

extern void SetCallsignID (int id, int num);

extern void UnsetCallsignID (int id, int num);

extern void GetCallsign (int id, int num, _TCHAR* callsign);

extern void GetCallsign (Flight fl, _TCHAR* callsign);

extern void GetDogfightCallsign (Flight flight);

#endif
