// OW - Cowboy bug (weird optimizer problem, reevaluate with new compiler)
#pragma optimize( "", off )

//
// Campaign Pilot and callsign information routines
//

#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include "ClassTbl.h"
#include "CampStr.h"
#include "Pilot.h"
#include "AIInput.h"
#include "F4Find.h"
#include "Campaign.h"
#include "Find.h"
#include "Flight.h"
#include "CmpClass.h"
#include "MissEval.h"
#include "falcsnd\voicemapper.h"

// ===========================
// Defines
// ===========================

#define NUM_CALLSIGNS		160
#define NUM_PILOTS			686
#define FIRST_PILOT_ID		2300
#define FIRST_CALLSIGN_ID	2000

// =======================
// The pilot array
// =======================

PilotInfoClass* PilotInfo = NULL;
uchar* CallsignData = NULL;

int NumPilots = 0;
int NumCallsigns = 0;

// =======================
// Externals
// =======================

void SetPilotStatus (Squadron sq, int pn);
void UnsetPilotStatus (Squadron sq, int pn);
extern uchar AssignUIImageID(uchar voice_id); // Defined in teamdata.cpp

extern uchar calltable[5][5];

// =======================
// Pilot Class
// =======================

PilotClass::PilotClass (void)
	{
	}

// 2000-11-17 MODIFIED BY S.G. I NEED TO PASS THE 'airExperience'.
// void PilotClass::ResetStats (void)
void PilotClass::ResetStats (uchar airExperience)
	{
	pilot_id = 0;
	pilot_status = PILOT_AVAILABLE;
// MODIFIED BY S.G. SO PILOT SKILL ARE NOT JUST VETERAN AND ACE BUT BASED ON THE SQUADRON SKILL +-1
//	pilot_skill_and_rating = 0x03 | (rand()%PILOT_SKILL_RANGE);
	airExperience -= 60; // From 60 to 100 (recruit to ace) down to 0 to 40
	airExperience /= 10; // Now from 0 to 4 like 'pilot_skill_and_rating' likes it
	pilot_skill_and_rating = 0x30 | ((rand() % 3 - 1) + airExperience); // pilot_skill_and_rating will have +-1 from 'airExperience' base level
// END OF MODIFIED SECTION
	aa_kills = 0;
	ag_kills = 0;
	as_kills = 0;
	an_kills = 0;
	missions_flown = 0;
	}

void PilotClass::SetTEPilotRating (uchar rating)		// M.N. for TE
{
	int skill;
	skill = ((rand() %2 - 1) + rating);	// randomize the squad's pilots a bit
	if (skill > 4)
		skill = 4;
	if (skill < 0)
		skill = 0;
	// pilot_id = 0; // 2002-01-24 REMOVED BY S.G. This is the pilot ID used to figure out the name of the pilot (and may be its voice). It's already set and if reset, it will remain at 0 (Unassigned).
	pilot_status = PILOT_AVAILABLE;
	pilot_skill_and_rating = skill;		
	aa_kills = 0;
	ag_kills = 0;
	as_kills = 0;
	an_kills = 0;
	missions_flown = 0;
}

// =======================
// Pilot Info Class
// =======================

PilotInfoClass::PilotInfoClass (void)
	{
	}

void PilotInfoClass::ResetStats (void)
{
    voice_id = 255;
    //g_voicemap.PickVoice(VoiceMapper::VOICE_PILOT, VoiceMapper::VOICE_SIDE_UNK);
    photo_id = 0;
    //AssignUIImageID(voice_id); // UI will check if this is male or female (& keep track of images)
    usage = 0;
}

void PilotInfoClass::AssignVoice(int owner)
{
    voice_id = g_voicemap.PickVoice(VoiceMapper::VOICE_PILOT, owner);
    photo_id = AssignUIImageID(voice_id); // UI will check if this is male or female (& keep track of images)
}

// =======================
// Functions
// =======================

void NewPilotInfo (void)
	{
	NumPilots = NUM_PILOTS;
	PilotInfo = new PilotInfoClass[NumPilots];
	for (int i=0; i<NumPilots; i++)
		PilotInfo[i].ResetStats();
	// Some hard coded values (Because I'm vain)
	// Col. Klemmick
	PilotInfo[1].voice_id = 1;
	PilotInfo[1].photo_id = 55;
	// Col. Bonanni
	PilotInfo[2].voice_id = 2;
	PilotInfo[2].photo_id = 102;
	// Col. Reiner
	PilotInfo[3].voice_id = 3;
	PilotInfo[3].photo_id = 77;
	// Unassigned pilot (don't use)
	PilotInfo[255].usage = 32000;
	NumCallsigns = NUM_CALLSIGNS;
	CallsignData = new unsigned char[NumCallsigns];
	memset(CallsignData,0,sizeof(uchar)*NumCallsigns);
	}

int LoadPilotInfo (char* scenario)
	{
	char	/* *data,*/ *data_ptr;
	short	max;

	if (gCampDataVersion < 60) {
		NewPilotInfo();
		return 1;
	}

	CampaignData cd = ReadCampFile(scenario, "plt");
	if (cd.dataSize == -1){
		return 0;
	}

	data_ptr = cd.data;

	// Pilot Data
	max = *((short *) data_ptr); data_ptr += sizeof (short);

	if (PilotInfo)
		delete [] PilotInfo;

	NumPilots = max;
	PilotInfo = new PilotInfoClass[NumPilots];

	memcpy (PilotInfo, data_ptr, sizeof (PilotInfoClass) * max); data_ptr += sizeof (PilotInfoClass) * max;

	// Callsign Data
	max = *((short *) data_ptr); data_ptr += sizeof (short);

	if (CallsignData)
		delete [] CallsignData;

	NumCallsigns = max;

	CallsignData = new unsigned char[NumCallsigns];
	memcpy (CallsignData, data_ptr, sizeof (uchar) * NumCallsigns); data_ptr += sizeof (uchar) * NumCallsigns;

	delete cd.data;
	return 1;
	}

void SavePilotInfo (char* scenario)
	{
	FILE	*fp;

	if ((fp = OpenCampFile(scenario, "plt", "wb")) == NULL)
		return;
	fwrite(&NumPilots,sizeof(short),1,fp);
	fwrite(PilotInfo,sizeof(PilotInfoClass),NumPilots,fp);
	fwrite(&NumCallsigns,sizeof(short),1,fp);
	fwrite(CallsignData,sizeof(uchar),NumCallsigns,fp);
	CloseCampFile(fp);
	}

void DisposePilotInfo (void)
	{
	if (PilotInfo)
		delete[] PilotInfo;
	PilotInfo = NULL;
	NumPilots = 0;
	if (CallsignData)
		delete [] CallsignData;
	CallsignData = NULL;
	NumCallsigns = 0;
	}

int GetAvailablePilot (int first, int last, int owner)
	{
	int		best_pilot = -1;
	ushort	best = ~0;

	if (last > NumPilots)
		last = NumPilots;

	for (int i=first; i<last; i++)
		{
		if (PilotInfo[i].usage < best)
			{
			best = PilotInfo[i].usage;
			best_pilot = i;
			}
		}
	
	if (best_pilot > -1)
		PilotInfo[best_pilot].usage++;

	if (PilotInfo[best_pilot].voice_id == 255) {
	    PilotInfo[best_pilot].AssignVoice(owner);
	}
	return best_pilot;
	}

/*
int PilotAvailable (int pn)
	{
	if (pn >= NumPilots)
		return 0;
	if (!PilotData[pn].flags)
		return 1;
	return 0;
	}

int GetPilotStatus (int pn)
	{
	if (pn >= NumPilots)
		return PILOT_KIA;
	return PilotData[pn].flags;
	}

int GetPilotVoiceId (int pn)
	{
	if (pn >= NumPilots)
		return 0;
	return PilotData[pn].voice_id;
	}

void SetPilotStatus (int pn, int f)
	{
	if (pn >= NumPilots)
		return;
	PilotData[pn].flags |= f;
	}

void UnsetPilotStatus (int pn, int f)
	{
	if (pn >= NumPilots)
		return;
	PilotData[pn].flags |= f;
	PilotData[pn].flags ^= f;
	}
*/

void GetPilotName (int id, _TCHAR* name, int size)
	{
	ReadIndexedString(FIRST_PILOT_ID+id,name,size);
	}

void GetCallsignID (uchar* id, uchar* num, int range)
	{
	int			i,j;

	for (j=1; j<9; j++)
		{
		for (i=(int)*id; i<(int)*id+range; i++)
			{
			if (i < NumCallsigns && !((CallsignData[i] >> (j-1)) & 0x01))
				{
				*id = (uchar)i;
				*num = (uchar)j;
				return;
				}
			}
		}
	// KCK: No callsigns left, pick one of the available ones with a '9'
	*num = 9;
	for (i=(int)*id; i<(int)*id+range; i++)
		{
		if (i < NumCallsigns && !(rand()%range))
			{
			*id = (uchar)i;
			return;
			}
		}
	}

void SetCallsignID (int id, int num)
	{
	int		temp = (0x01 << (num-1));
	CallsignData[id] |= (uchar)(temp);
	}

void UnsetCallsignID (int id, int num)
	{
	int		temp = (0x01 << (num-1));
	CallsignData[id] &= (uchar)(~temp);
	}

void GetCallsign (int id, int num, _TCHAR* callsign)
	{
	_TCHAR		wname[30];

	if (num)
		{
		ReadIndexedString(FIRST_CALLSIGN_ID+id,callsign,29);
		ReadIndexedString(num,wname,29);
		_tcscat(callsign,wname);
		}
	}

void GetCallsign (Flight fl, _TCHAR* callsign)
	{
	_TCHAR		wname[30];
	int			id, num;

	id = fl->callsign_id;
	num = fl->callsign_num;
	if (num)
		{
		ReadIndexedString(FIRST_CALLSIGN_ID+id,callsign,29);
		ReadIndexedString(num,wname,29);
		_tcscat(callsign,wname);
		}
	else
		{
		VehicleClassDataType *vc;
		vc = GetVehicleClassData(fl->GetVehicleID(0));
		_stprintf(callsign,vc->Name);
		}
	}

void GetDogfightCallsign (Flight flight)
	{
	int	num,i,checkid;

	for (num=1; num<9; num++)
		{
		for (i=0; i<5; i++)
			{
			checkid = calltable[flight->GetTeam()][i];
			if (checkid < NumCallsigns && !((CallsignData[checkid] >> (num-1)) & 0x01))
				{
				flight->callsign_id = (uchar)checkid;
				flight->callsign_num = (uchar)num;
				MonoPrint ("Flight %08x Callsign %d:%d\n", flight, checkid, num);
				return;
				}
			}
		}
	}
