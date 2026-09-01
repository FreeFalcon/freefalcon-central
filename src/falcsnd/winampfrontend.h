/*****************************************************************************/
// Filename: winampfrontend.h
// Author: Retro
// Date: 3Jan2004
// Description:see .cpp
/*****************************************************************************/

/* "ONE IN THE EYE OF THE BEAUTIFUL PEOPLE.." */

#ifndef WINAMP_FRONTEND_INCLUDED
#define WINAMP_FRONTEND_INCLUDED

#include "sim/include/stdhdr.h"

#include "sim/include/icp.h" // for MAX_DED_LEN

// because I start at DED pos 1 with my strings
#define MY_MAX_DED_LEN (MAX_DED_LEN - 1)

class WinAmpFrontEnd
{
public:
#ifdef _WIN32
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
#else
    // Linux: Winamp (a Windows media player driven via its window messages) does not exist. The subsystem is
    // inert -- `winamp` is always NULL and every call site is guarded by `if (winamp)` -- so inline no-ops keep
    // those dead branches linking without the Windows-only winampfrontend.cpp.
    WinAmpFrontEnd()
    {
    }
    ~WinAmpFrontEnd()
    {
    }
    void InitWinAmp()
    {
    }
    void StopAndFadeout()
    {
    }
    void Previous()
    {
    }
    void Start()
    {
    }
    void Stop()
    {
    }
    void Next()
    {
    }
    void VolUp()
    {
    }
    void VolDown()
    {
    }
    void TogglePlayback()
    {
    }
    char* getCurTitle()
    {
        return 0;
    }
    char* getDEDTitle(const int)
    {
        return 0;
    }
    int getVolume()
    {
        return 0;
    }
    void Refresh(unsigned long)
    {
    }
#endif

private:
    HWND winamp_win;
    bool ampexists;

    void copyCurTitle();

    char* currentTrackTitle;

    // the title track in the DED - split into 2 lines,
    // if the title is longer it�s cut off
    char DEDString[2][MY_MAX_DED_LEN];

    int volume; // value between 0-255

    unsigned long myTimer;
    int WinAmpAlive;
};
extern WinAmpFrontEnd* winamp;

#endif WINAMP_FRONTEND_INCLUDED
