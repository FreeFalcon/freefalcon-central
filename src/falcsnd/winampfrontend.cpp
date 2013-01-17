/*****************************************************************************/
//	Filename:	winampfrontend.cpp
//	Author:		Retro
//	Date:		3Jan2004
//	Description:Used the winamp 2.xx API to communicate with said application
//				Provides some functions that are currently used within the
//				DED/ICP code, they can however be bound to keyboard commands
//				as well..
/*****************************************************************************/
#include "frontend.h"		// Retro - Winamp functionality
#include "winampfrontend.h"

#define REFRESH_INTERVAL 2000	// a check every 2 seconds if the title changed..
#define INITIAL_VOLUME 204		// 80% max volume

extern int g_nWinAmpInitVolume;

/*****************************************************************************/
//	Constructor..
/*****************************************************************************/
WinAmpFrontEnd::WinAmpFrontEnd()
{
	winamp_win = 0;
	currentTrackTitle = 0;

	for (int i = 0; i < 2; i++)
		sprintf(DEDString[i],"Not initialized");

	if ((g_nWinAmpInitVolume >= 0)&&(g_nWinAmpInitVolume <= 255))
		volume = g_nWinAmpInitVolume;
	else
		volume = INITIAL_VOLUME;

	myTimer = 0;
	WinAmpAlive = 0;
};

/*****************************************************************************/
//	Destruktor..
/*****************************************************************************/
WinAmpFrontEnd::~WinAmpFrontEnd()
{
	free(currentTrackTitle);
	currentTrackTitle = 0;
};

/*****************************************************************************/
//	Initialisation. Searches for the winamp window. I might want to call
//	this on entering the 3d so that the user doesn´t have to restart falcon
//	if he accidentially canceled (or didn´t open in the first place) the winamp
//	window
/*****************************************************************************/
void WinAmpFrontEnd::InitWinAmp()
{
	// Find a window with classname "Winamp v1.x"
	// Returns NULL if not found
	winamp_win = FindWindow("Winamp v1.x",NULL);		
	if (winamp_win == NULL)
		ampexists=false;
	else
	{
		ampexists=true;
		// preinit volume (to 80% for now), so that I can sync
		// that with the DED display (via the 'volume' variable
		SendMessage(winamp_win,WM_USER, volume, 122);	
	}

	for (int i = 0; i < 2; i++)
		sprintf(DEDString[i],"Not initialized");

	myTimer = 0;
	WinAmpAlive = 0;
}

/*****************************************************************************/
//	Fades out (5 seconds timer) and stops playback. I might call this on
//	exitting the 3d..
/*****************************************************************************/
void WinAmpFrontEnd::StopAndFadeout()
{
	if (!ampexists)
		return;

	copyCurTitle();
	SendMessage(winamp_win, WM_COMMAND,WINAMP_BUTTON4_SHIFT,0);
}
/*****************************************************************************/
//	Plays previous track in playlists, or restarts single track
/*****************************************************************************/
void WinAmpFrontEnd::Previous()
{
	if (!ampexists)
		return;

	copyCurTitle();
	SendMessage(winamp_win, WM_COMMAND,WINAMP_BUTTON1,0);
}
/*****************************************************************************/
//	Starts Playback.
/*****************************************************************************/
void WinAmpFrontEnd::Start()
{
	if (!ampexists)
		return;

	copyCurTitle();
	SendMessage(winamp_win, WM_COMMAND,WINAMP_BUTTON2,0);
}
/*****************************************************************************/
//	Stops Playback
/*****************************************************************************/
void WinAmpFrontEnd::Stop()
{
	if (!ampexists)
		return;

	copyCurTitle();
	SendMessage(winamp_win, WM_COMMAND,WINAMP_BUTTON4,0);
}
/*****************************************************************************/
//	Plays next track in playlist, or restarts single track
/*****************************************************************************/
void WinAmpFrontEnd::Next()
{
	if (!ampexists)
		return;

	copyCurTitle();
	SendMessage(winamp_win, WM_COMMAND,WINAMP_BUTTON5,0);
}
/*****************************************************************************/
//	Increasy volume by 1%
/*****************************************************************************/
void WinAmpFrontEnd::VolUp()
{
	if ((ampexists)&&(volume<255))
	{
		volume++;
		SendMessage(winamp_win, WM_COMMAND,WINAMP_VOLUMEUP,0);
	}
}
/*****************************************************************************/
//	Decrease volume by 1%
/*****************************************************************************/
void WinAmpFrontEnd::VolDown()
{
	if ((ampexists)&&(volume>0))
	{
		volume--;
		SendMessage(winamp_win, WM_COMMAND,WINAMP_VOLUMEDOWN,0);
	}
}
/*****************************************************************************/
//	Toggles Playback. Pause is interpreted as 'stopped' for now, so there is
//	no resume but a complete start of the paused track
/*****************************************************************************/
void WinAmpFrontEnd::TogglePlayback()
{
	if (!ampexists)
		return;

	int ret=SendMessage(winamp_win,WM_USER, 0, 104);
	switch (ret)
	{
		case 1:	Stop(); break;
		default:
		case 3: Start(); break;	// actually, 3 is the 'paused' status.. but whatever..
	}
	copyCurTitle();
}
/*****************************************************************************/
//	Copies title of currently played track into a class-internal (heap) string
//	Also copies this title into two class internal strings that are
//	set up to be displayed in the DED (they have the correct length)
//
//	the upper part is ripped straight out of the winamp tutorial, and not
//	really optimized ?
//
//	this function should be called on start/stop/next/previous/toggle
//	IN ADDITION should be called at fixed intervals (every 2 seconds ?)
/*****************************************************************************/
void WinAmpFrontEnd::copyCurTitle()
{
	if (!ampexists)	// should really not be necessary..
		return;

	if (currentTrackTitle)
	{
		free(currentTrackTitle);
		currentTrackTitle = 0;
	}

	char this_title[512],*p;
	int len = 0;

	len = GetWindowText(winamp_win,this_title,sizeof(this_title));
	if (len == 0)
		return;

	p = this_title+strlen(this_title)-8;
	
	while (p >= this_title)
	{
		if (!strnicmp(p,"- Winamp",8))
		{
			break;
		}
		p--;
	}
	if (p >= this_title)
	{
		p--;
	}
	
	while (p >= this_title && *p == ' ')
	{
		p--;
	}
	*++p=0;

	currentTrackTitle = (char*)malloc(strlen(this_title)+1);
	if (currentTrackTitle)
		strcpy(currentTrackTitle,this_title);

	// copying DED String 1
	strncpy(&DEDString[0][0],this_title,MY_MAX_DED_LEN-1);
	DEDString[0][MY_MAX_DED_LEN-1] = '\0';
	// ..if the title is longer, fill the rest in to DEDString 2
	if (strlen(this_title)>MY_MAX_DED_LEN-1)
	{
		strncpy(&DEDString[1][0],&this_title[MY_MAX_DED_LEN-1],MY_MAX_DED_LEN-1);
		DEDString[1][MY_MAX_DED_LEN-1] = '\0';
	}
	else	// else don´t print it
	{
		DEDString[1][0] = '\0';
	}
}
/*****************************************************************************/
//	returns pointer to string of currently played title. this string gets
//	deleted in this class, so no action by the user is required
/*****************************************************************************/
char* WinAmpFrontEnd::getCurTitle()
{
	if (!ampexists)
		return "No WinAMP 2.xx window found !";

	return currentTrackTitle;	
}
/*****************************************************************************/
//	returns pointer to string of currently played title.
//
//	line 0 is first chunk of title, line 1 second chunk. every chunk has the
//	correct length for the DED. if the title is longer than 2*chunklength, it
//	is cut off
/*****************************************************************************/
char* WinAmpFrontEnd::getDEDTitle(const int theLine)
{
	if (!ampexists)
		return "No WinAMP window found !";
		
	if ((theLine != 0)&&(theLine != 1))
		return 0;

	return DEDString[theLine];
}
/*****************************************************************************/
//	returns current volume. 0 <= volume <= 255
/*****************************************************************************/
int WinAmpFrontEnd::getVolume()
{
	if (!ampexists)
		return 0;

	return volume;
}
/*****************************************************************************/
//	should be called periodically to refresh the display of the currently
//	played track in the DED. of course, if no DED is being displayed, this
//	function need not be called...
/*****************************************************************************/
void WinAmpFrontEnd::Refresh(unsigned long timer)
{
	if (!ampexists)
		return;

	if (timer > myTimer)
	{
		copyCurTitle();
		myTimer = timer + REFRESH_INTERVAL;

		WinAmpAlive++;	// once every 40 secs I check if winamp was deactivated..
		WinAmpAlive %= 20;
		if (WinAmpAlive == 0)
		{
			if (FindWindow("Winamp v1.x",NULL) == NULL)
			{
				ampexists = false;
			}
		}
	}
}
