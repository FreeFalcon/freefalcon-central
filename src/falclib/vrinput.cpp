#include "VRInput.h"

#ifdef VOICE_INPUT

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "shi/ShiError.h"
#include "MsgInc/AwacsMsg.h"
#include "VRstdafx.h"
#include <windows.h>
#include "VRlib.h"
#include "VRmsgid.h"
#include "Wingorder.h"
#include "chandler.h"
#include "cmusic.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
	wing1,
	wing2,
	wing3,
	wing4,
	wing5,
	wing6,
	wing7,
	wing8,
	wing10,
	sent6,
	sent7,
	sent8,
	sent9,
	sent10,
	tower1,
	tower2,
	tower3,
	tower4,
	tower5;

extern CSoundMgr *gSoundDriver;
extern void MenuSendAwacs(int enumId, VU_ID targetId, int sendRequest = TRUE);
extern void MenuSendAtc(int enumId, int sendRequest = TRUE);
extern void MenuSendWingman(int enumId, int extent);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void InitVoiceRecognitionEngine (void)
{
	if (!InitRecoEngine())
		ShiAssert(!"Error initializing speech engine\nPlease check your registry entries.");
	
	//	int iNumTopics = CompileTopics(true);
	
	//	if (iNumTopics == 0)
	//		ShiAssert(!"Application error. Unable to compile topic (*.tpc) files.");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void InitVoiceRecognitionTopicsFiles (char *path)
{
	//	int iNumTopics = CompileTopics(true);
	int iNumTopics = CompileTopics(true, path);
	
	if (iNumTopics == 0)
		ShiAssert(!"Application error. Unable to compile topic (*.tpc) files.");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void load_voice_recognition_demo_sound_file (void)
{
	wing1 = F4LoadSound ("Wingman1.wav", 0);
	wing2 = F4LoadSound ("Wingman2.wav", 0);
	wing3 = F4LoadSound ("Wingman3.wav", 0);
	wing4 = F4LoadSound ("Wingman4.wav", 0);
	wing5 = F4LoadSound ("Wingman5.wav", 0);
	wing6 = F4LoadSound ("Wingman6.wav", 0);
	wing7 = F4LoadSound ("Wingman7.wav", 0);
	wing8 = F4LoadSound ("Wingman8.wav", 0);
	wing10 = F4LoadSound ("Wingman10.wav", 0);
	sent6 = F4LoadSound ("Sentry6.wav", 0);
	sent7 = F4LoadSound ("Sentry7.wav", 0);
	sent8 = F4LoadSound ("Sentry8.wav", 0);
	sent9 = F4LoadSound ("Sentry9.wav", 0);
	sent10 = F4LoadSound ("Sentry10.wav", 0);
	tower1 = F4LoadSound ("Haemitwr1.wav", 0);
	tower2 = F4LoadSound ("Haemitwr2.wav", 0);
	tower3 = F4LoadSound ("Haemitwr3.wav", 0);
	tower4 = F4LoadSound ("Haemitwr4.wav", 0);
	tower5 = F4LoadSound ("Haemitwr5.wav", 0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DoVoiceRecognitionInput (void)
{
	static int
		delay_wing1 = 0,
		delay_wing3 = 0,
		delay_wing8 = 0,
		delay_wing10 = 0,
		delay_sent8 = 0,
		delay_sent9 = 0,
		delay_sent10 = 0,
		delay_tower1 = 0,
		delay_tower3 = 0,
		idle_before = FALSE;

	char
		aDummy[255];

	int
		now,
		iRecoResult;

	now = GetTickCount ();
	
	if ((delay_wing1) && (now > delay_wing1)) { F4PlaySound (wing1); delay_wing1 = 0; }
	if ((delay_wing3) && (now > delay_wing3)) { F4PlaySound (wing3); delay_wing3 = 0; }
	if ((delay_wing8) && (now > delay_wing8)) { F4PlaySound (wing8); delay_wing8 = 0; }
	if ((delay_wing10) && (now > delay_wing10)) { F4PlaySound (wing10); delay_wing10 = 0; }
	if ((delay_sent8) && (now > delay_sent8)) { F4PlaySound (sent8); delay_sent8 = 0; }
	if ((delay_sent9) && (now > delay_sent9)) { F4PlaySound (sent9); delay_sent9 = 0; }
	if ((delay_sent10) && (now > delay_sent10)) { F4PlaySound (sent10); delay_sent10 = 0; }
	if ((delay_tower1) && (now > delay_tower1)) { F4PlaySound (tower1); delay_tower1 = 0; }
	if ((delay_tower3) && (now > delay_tower3)) { F4PlaySound (tower3); delay_tower3 = 0; }
	
	iRecoResult = GetRecoEvent(aDummy,30);

	switch (iRecoResult)
	{
	case VR_ENGINE_ERROR:
		MonoPrint ("VR Engine Error\n");
		break;
		
	case VR_UNABLE_TO_RECONIZE:
		MonoPrint ("VR Unable To Reconize\n");
		break;
		
	case VR_IDLE:
		if (!idle_before)
		{
			idle_before = TRUE;
			delay_tower1 = now + 2000;
			break;
		}
		break;

	case 10000:	// radio check
		MonoPrint ("Radio Check\n");
		F4PlaySound (tower5);
		break;

	case 10001: // My wingman will go ahead
		MonoPrint ("My wingman will go ahead\n");
		F4PlaySound (tower2);
		delay_wing1 = now + 2000 + (rand() % 1000);
		delay_tower3 = delay_wing1 + 3000 + (rand() % 1000);
		delay_wing10 = delay_tower3 + 5000 + (rand() % 1000);
		break;

	case 10002: // Ready for takeoff
		MonoPrint ("Ready for takeoff\n");
		F4PlaySound (tower4);
		break;

	case 10003: // join on my right wing
		MonoPrint ("Join on my right wing\n");
		F4PlaySound (wing2);
		delay_wing3 = now + 5000 + (rand() % 2000);
		break;

	case 10004: // button 5
		MonoPrint ("Button 5\n");
		F4PlaySound (wing4);
		break;

	case 10005: // zero three zero
		MonoPrint ("Heading zero three zero\n");
		F4PlaySound (sent6);
		break;

	case 10006: // one eight zero cowboy one
		MonoPrint ("...180 cowboy one\n");
		F4PlaySound (sent7);
		delay_sent8 = now + 5000 + (rand() % 3000);
		delay_sent9 = delay_sent8 + 15000 + (rand() % 5000);
		delay_sent10 = delay_sent9 + 15000 + (rand() % 5000);
		break;

	case 10007: // burners now
		MonoPrint ("Go burners now\n");
		F4PlaySound (wing5);
		break;

	case 10008: // 80 percent
		MonoPrint ("80%\n");
		F4PlaySound (wing6);
		break;

	case 10009: // in sight ?
		MonoPrint ("Two do you have em in sight\n");
		F4PlaySound (wing7);
		delay_wing8 = now + 2000 + (rand() % 3000);
		break;

	default:
		
		if (iRecoResult >= AWACS_COMMANDS && iRecoResult < WINGMEN_COMMANDS)
		{
			MenuSendAwacs(iRecoResult - AWACS_COMMANDS, FalconNullId, FALSE);
		}
		if (iRecoResult >= ATC_COMMANDS && iRecoResult < WINGMEN_COMMANDS)
		{
			MenuSendAtc(iRecoResult - ATC_COMMANDS, FALSE);
		}
		if (iRecoResult >= WINGMEN_COMMANDS && iRecoResult < LAST_COMMAND)
		{
			MenuSendWingman(iRecoResult - WINGMEN_COMMANDS, AiAllButSender);
			//				MenuSendWingman(iRecoResult - WINGMEN_COMMANDS, AiWingman);
			//				MenuSendWingman(iRecoResult - WINGMEN_COMMANDS, AiElement);
		}
		
		break;
		
		/*		case MSG_EXIT:
		MonoPrint("Exiting application");
		break;
		
		  case MSG_SHOW_PHRASES:
		  StreamTopics();
		  break;
		  
			case MSG_HELP:
			MonoPrint("\nSay 'SHOW PHRASES' to see a list of phrases\n");
			MonoPrint("Speaking any of the phrases will display its associated value\n");
			MonoPrint("Say 'EXIT APPLICATION' to quit\n");
			break;
		*/
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#endif
