/*****************************************************************************/
//	Filename:	winampfrontend.h
//	Author:		Retro
//	Date:		3Jan2004
//	Description:see .cpp
/*****************************************************************************/

/* "ONE IN THE EYE OF THE BEAUTIFUL PEOPLE.." */

#ifndef WINAMP_FRONTEND_INCLUDED
#define WINAMP_FRONTEND_INCLUDED

#include "sim\include\stdhdr.h"

#include "sim\include\Icp.h"	// for MAX_DED_LEN

// because I start at DED pos 1 with my strings
#define MY_MAX_DED_LEN (MAX_DED_LEN-1)

class WinAmpFrontEnd {
public:
	WinAmpFrontEnd();
	~WinAmpFrontEnd();

	void InitWinAmp();

	void StopAndFadeout();
	void Previous();
	void Start();
	void Stop();
	void Next();
	void VolUp();
	void VolDown();
	void TogglePlayback();

	char* getCurTitle();
	char* getDEDTitle(const int theLine);
	int getVolume();

	void Refresh(unsigned long timer);

private:
	HWND winamp_win;
	bool ampexists;

	void copyCurTitle();

	char* currentTrackTitle;

	// the title track in the DED - split into 2 lines,
	// if the title is longer it´s cut off
	char DEDString[2][MY_MAX_DED_LEN];

	int volume;			// value between 0-255

	unsigned long myTimer;
	int WinAmpAlive;
};
extern WinAmpFrontEnd* winamp;

#endif WINAMP_FRONTEND_INCLUDED